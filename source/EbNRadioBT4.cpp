#include "EbNRadioBT4.h"

#include <stdexcept>

#include "AndroidWake.h"
#include "RSErasureDecoder.h"
#include "Timing.h"

using namespace std;

EbNRadio::ConfirmScheme EbNRadioBT4::getDefaultConfirmScheme()
{
  return {ConfirmScheme::Passive, 0.05};
}

EbNRadioBT4::EbNRadioBT4(size_t keySize, ConfirmScheme confirmScheme, MemoryScheme memoryScheme, int adapterID)
   : EbNRadio(keySize, confirmScheme, memoryScheme),
     RS_W(computeRSSymbolSize(keySize, (31 * 8) - 1 - ADV_N_LOG2)),
     RS_K((keySize / 8) / RS_W),
     RS_M(ADV_N - RS_K),
     BF_SM((31 * 8) - 1 - ADV_N_LOG2 - (RS_W * 8)),
     BF_K(1),
     BF_B(2),
     hci_(adapterID),
     deviceMap_(),
     dhCodeMatrix_(RS_K, RS_M + ((SCAN_ACTIVE ? 2 : 1) * (RS_K - 1)), RS_W),
     dhEncoder_(dhCodeMatrix_),
     dhPrevSymbols_(((SCAN_ACTIVE ? 2 : 1) * (RS_K - 1)) * RS_W),
     dhExchange_(keySize),
     dhExchangeMutex_(),
     advertNum_(0),
     advertBloom_(),
     advertBloomNum_(-1),
     listenThread_(),
     listenAdverts_(),
     listenAdvertsMutex_()
{
  LOG_D("EbNRadioBT4", "General Parameters: ADV_N = %zu, ADV_N_LOG2 = %zu", ADV_N, ADV_N_LOG2);
  LOG_D("EbNRadioBT4", "RS Parameters: W = %zu, K = %zu, M = %zu", RS_W, RS_K, RS_M);
  LOG_D("EbNRadioBT4", "BF Parameters: SM = %zu", BF_SM);

  dhEncoder_.encode(dhExchange_.getPublicX());
}

void EbNRadioBT4::initialize()
{
  // Disabling advertising so that we can set up the new address and first
  // advertisement before remote devices can receive it
  hci_.enableAdvertising(false);

  uint8_t partial = (uint8_t)dhExchange_.getPublicY() << 5;
  hci_.setRandomAddress(Address::generateWithPartial(6, partial, 0x20));

  changeAdvert();
  hci_.enableAdvertising(true);

  // Start a thread to listen for incoming connections in the case of active or
  // hybrid confirmation schemes
  if((confirmScheme_.type & ConfirmScheme::Active) != 0)
  {
    listenThread_ = thread(&EbNRadioBT4::listen, this);
  }
}

list<DiscoverEvent> EbNRadioBT4::discover()
{
  if(memoryScheme_ == MemoryScheme::NoMemory)
  {
    deviceMap_.clear();
  }

  changeAdvert();

  list<DiscoverEvent> discovered;
  function<void(const ScanResponse *)> callback = bind(&EbNRadioBT4::processScanResponse, this, &discovered, placeholders::_1);
  hci_.performScan(SCAN_ACTIVE ? BluetoothHCI::Scan::Active : BluetoothHCI::Scan::Passive, BluetoothHCI::DuplicateFilter::On, SCAN_WINDOW, callback);

  nextDiscover_ += SCAN_INTERVAL + (-1000 + (rand() % 2001));

  return discovered;
}

void EbNRadioBT4::changeEpoch()
{
  advertNum_ = 0;

  lock_guard<mutex> dhExchangeLock(dhExchangeMutex_);

  // Generate a new secret for this epoch's DH exchanges
  dhExchange_.generateSecret();

  // Copying the leftover symbols from the prior epoch, which we will later
  // include in the first K-1 adverts of this epoch
  for(int k = 0; k < (RS_K - 1); k++)
  {
    memcpy(dhPrevSymbols_.data() + (k * RS_W), dhEncoder_.getSymbol(RS_K + RS_M + k), RS_W);
  }

  // Generating new symbols based on an updated DH local public value
  dhEncoder_.encode(dhExchange_.getPublicX());

  // Shifting the current address. Uses the 1 bit Y coordinate as part of the
  // address since compressed ECDH keys are actually (keySize + 1) bits long
  uint8_t partial = (uint8_t)dhExchange_.getPublicY() << 5;
  hci_.setRandomAddress(hci_.getRandomAddress().shiftWithPartial(partial, 0x20));

  // Computing new shared secrets in the case of passive or hybrid confirmation
  if((confirmScheme_.type & ConfirmScheme::Passive) != 0)
  {
    for(auto it = deviceMap_.begin(); it != deviceMap_.end(); it++)
    {
      EbNDeviceBT4 *device = it->second;
      if(!device->epochs_.empty())
      {
        EbNDeviceBT4::Epoch &curEpoch = device->epochs_.back();
        if(curEpoch.dhDecoder.isDecoded())
        {
          SharedSecret sharedSecret(confirmScheme_.type == ConfirmScheme::None);
          if(dhExchange_.computeSharedSecret(sharedSecret, curEpoch.dhDecoder.decode(), curEpoch.dhExchangeYCoord))
          {
            device->addSharedSecret(sharedSecret);
          }
          else
          {
            LOG_E("EbNRadioBT4", "Could not compute shared secret for id %d", device->getID());
          }
        }
        else
        {
          curEpoch.dhExchanges.push_back(dhExchange_);
        }
      }
    }
  }

  nextChangeEpoch_ += EPOCH_INTERVAL;
}

set<DeviceID> EbNRadioBT4::handshake(const set<DeviceID> &deviceIDs)
{
  set<DeviceID> encountered;

  // Processing the results of any adverts that came in from other devices
  unique_lock<mutex> listenLock(listenAdvertsMutex_);

  for(auto it = listenAdverts_.begin(); it != listenAdverts_.end(); it++)
  {
    Address &address = it->first;
    vector<uint8_t> &message = it->second;

    EbNDeviceBT4 *device = deviceMap_.get(address);
    if(device == NULL)
    {
      lock_guard<mutex> setLock(setMutex_);

      device = new EbNDeviceBT4(generateDeviceID(), address, listenSet_);
      deviceMap_.add(address, device);

      LOG_P("EbNRadioBT4", "Discovered new EbN device via incoming connection (ID %d, Address %s)", device->getID(), device->getAddress().toString().c_str());
    }

    addRecentDevice(device);
    processActiveHandshake(message, device);

    device->setShakenHands(true);
  }

  listenAdverts_.clear();
  listenLock.unlock();

  // Attempting to confirm the selected set of devices
  for(auto it = deviceIDs.begin(); it != deviceIDs.end(); it++)
  {
    EbNDeviceBT4 *device = deviceMap_.get(*it);

    // Only perform active confirmation once per device
    if(!device->isConfirmed() && (getHandshakeScheme() == ConfirmScheme::Active))
    {
      // Making sure that we have enough time until our next action
      ActionInfo nextAction = getNextAction();
      if(nextAction.timeUntil < 2000)
      {
        break;
      }

      // Unconnectable advertisement type
      hci_.setUndirectedAdvertParams(getAdvertType(false), BluetoothHCI::AdvertFilter::ScanAllConnectAll, ADVERT_MIN_INTERVAL, ADVERT_MAX_INTERVAL);

      LOG_D("EbNRadioBT4", "Attempting to connect to remote device (ID %d, Address %s)...", device->getID(), device->getAddress().toString().c_str());

      int sock = hci_.connectBT4(device->getAddress(), 2000);
      if(sock >= 0)
      {
        // Note: We do not need to lock on dhExchange_ here since it can
        // only be modified by changeEpoch, which cannot be called while we
        // are handshaking.
        vector<uint8_t> localMessage = generateActiveHandshake(dhExchange_);
        vector<uint8_t> remoteMessage(localMessage.size(), 0);

        if(hci_.send(sock, localMessage.data(), localMessage.size(), 1000))
        {
          if(hci_.recv(sock, remoteMessage.data(), remoteMessage.size(), 1000))
          {
            BitMap remoteMessageBM(dhExchange_.getPublicSize() * 8, remoteMessage.data());
            LOG_D("EbNRadioBT4", "Received '%s' from remote device", remoteMessageBM.toHexString().c_str());

            // Sometimes we get an all zero message due to some failure, just ignore it if we do
            bool allZeros = true;
            for(int b = 0; b < remoteMessage.size(); b++)
            {
              if(remoteMessage.data()[b] != 0)
              {
                allZeros = false;
                break;
              }
            }

            if(!allZeros)
            {
              processActiveHandshake(remoteMessage, device);
            }
          }
        }
      }

      close(sock);

      // Connectable advertisement type
      hci_.setUndirectedAdvertParams(getAdvertType(true), BluetoothHCI::AdvertFilter::ScanAllConnectAll, ADVERT_MIN_INTERVAL, ADVERT_MAX_INTERVAL);
    }

    device->setShakenHands(true);
  }

  // Going through all devices to report 'encountered' devices, meaning
  // the devices we have shaken hands with and confirmed
  for(auto it = deviceMap_.begin(); it != deviceMap_.end(); it++)
  {
    EbNDevice *device = it->second;
    if(device->hasShakenHands() && device->isConfirmed())
    {
      encountered.insert(device->getID());
    }
  }

  return encountered;
}

EncounterEvent EbNRadioBT4::doneWithDevice(DeviceID id)
{
  EbNDeviceBT4 *device = deviceMap_.get(id);

  EncounterEvent expiredEvent(getTimeMS());
  device->getEncounterInfo(expiredEvent, true);

  deviceMap_.remove(id);
  removeRecentDevice(id);

  return expiredEvent;
}

BluetoothHCI::UndirectedAdvert EbNRadioBT4::getAdvertType(bool canAllowConnections)
{
  BluetoothHCI::UndirectedAdvert advertType;
  switch(confirmScheme_.type)
  {
  case ConfirmScheme::None:
  case ConfirmScheme::Passive:
    if(SCAN_ACTIVE)
    {
      advertType = BluetoothHCI::UndirectedAdvert::Scannable;
    }
    else
    {
      advertType = BluetoothHCI::UndirectedAdvert::Standard;
    }
    break;
  case ConfirmScheme::Active:
    if(canAllowConnections)
    {
      advertType = BluetoothHCI::UndirectedAdvert::Connectable;
    }
    else
    {
      advertType = BluetoothHCI::UndirectedAdvert::Scannable;
    }
    break;
  case ConfirmScheme::Hybrid:
    if((getHandshakeScheme() & ConfirmScheme::Passive) || !canAllowConnections)
    {
      advertType = BluetoothHCI::UndirectedAdvert::Scannable;
    }
    else
    {
      advertType = BluetoothHCI::UndirectedAdvert::Connectable;
    }
    break;
  default:
    throw runtime_error("Invalid confirmation scheme");
  }

  LOG_D("EbNRadioBT4", "Selecting advertisement type %d", advertType);

  return advertType;
}

BitMap EbNRadioBT4::generateAdvert(size_t advertNum)
{
  BitMap advert(31 * 8);
  size_t advertOffset = 0;

  advert.setAll(false);

  // NOTE: Version bit is 0, since we are not an infrastructure node
  advertOffset += 1;

  // Inserting the advertisement number as the first portion of the advert
  for(int b = 0; b < ADV_N_LOG2; b++)
  {
    advert.set(b + advertOffset, (advertNum >> b) & 0x1);
  }
  advertOffset += ADV_N_LOG2;

  // Insert the RS coded symbols for the DH public value. For the first K-1
  // adverts of an epoch, add in a symbol from the previous epoch. This allows a
  // user to decode if they receive any K consecutive packets
  advert.copyFrom(dhEncoder_.getSymbol(advertNum), 0, advertOffset, 8 * RS_W);
  advertOffset += 8 * RS_W;

  if(advertNum < (RS_K - 1))
  {
    advert.copyFrom(dhPrevSymbols_.data() + (advertNum * RS_W), 0, advertOffset, 8 * RS_W);
    advertOffset += 8 * RS_W;
  }

  // Computing a new Bloom filter every BF_B advertisements
  uint32_t bloomNum = advertNum / BF_B;

  if(((advertNum % BF_B) == 0) || (bloomNum != advertBloomNum_))
  {
    BitMap prefix(ADV_N_LOG2 + keySize_);
    for(int b = 0; b < ADV_N_LOG2; b++)
    {
      prefix.set(b, (bloomNum >> b) & 0x1);
    }
    prefix.copyFrom(dhExchange_.getPublicX(), 0, ADV_N_LOG2, keySize_);

    uint32_t totalSize = 0;
    vector<uint32_t> segmentSizes;
    for(int s = bloomNum * BF_B; s < (bloomNum + 1) * BF_B; s++)
    {
      uint32_t segmentSize = (s < (RS_K - 1)) ? (BF_SM - (8 * RS_W)) : BF_SM;
      segmentSizes.push_back(segmentSize);
      totalSize += segmentSize;
    }

    advertBloom_ = SegmentedBloomFilter(BF_N, totalSize, BF_K, BF_B, segmentSizes);
    fillBloomFilter(&advertBloom_, prefix.toByteArray(), prefix.sizeBytes());
    advertBloomNum_ = bloomNum;
  }

  advertBloom_.getSegment(advertNum % BF_B, advert.toByteArray(), advertOffset);

  return advert;
}

void EbNRadioBT4::changeAdvert()
{
  if(advertNum_ >= ADV_N)
  {
    LOG_D("EbNRadioBT4", "Reached last unique advert, waiting for epoch change\n");
    return;
  }

  BluetoothHCI::UndirectedAdvert advertType = getAdvertType();

  // Setting the advertisement data to periodically broadcast
  BitMap advert = generateAdvert(advertNum_);
  hci_.setAdvertData(advert.toByteArray(), 31);
  LOG_P("EbNRadioBT4", "Setting advert %d data to \'%s\'", advertNum_, advert.toHexString().c_str());
  advertNum_++;

  // Setting the scan response data if active scanning is enabled
  if(SCAN_ACTIVE)
  {
    BitMap scanResponse = generateAdvert(advertNum_);
    hci_.setScanResponseData(scanResponse.toByteArray(), 31);
    LOG_P("EbNRadioBT4", "Setting scan response advert %d data to \'%s\'", advertNum_, scanResponse.toHexString().c_str());
    advertNum_++;
  }

  // Updating the advertisement parameters if the type has changed
  hci_.setUndirectedAdvertParams(advertType, BluetoothHCI::AdvertFilter::ScanAllConnectAll, ADVERT_MIN_INTERVAL, ADVERT_MAX_INTERVAL);
}

void EbNRadioBT4::processScanResponse(list<DiscoverEvent> *discovered, const ScanResponse *resp)
{
  uint64_t scanTime = getTimeMS();

  bool addressOK = resp->address.verifyChecksum();
  bool lengthOK = (resp->length == 31);
  if(addressOK && lengthOK)
  {
    EbNDeviceBT4 *device = deviceMap_.get(resp->address);
    if(device == NULL)
    {
      lock_guard<mutex> setLock(setMutex_);

      device = new EbNDeviceBT4(generateDeviceID(), resp->address, listenSet_);
      deviceMap_.add(resp->address, device);

      LOG_P("EbNRadioBT4", "Discovered new EbN device (ID %d, Address %s)", device->getID(), device->getAddress().toString().c_str());
    }

    device->addRSSIMeasurement(scanTime, resp->rssi);
    discovered->push_back(DiscoverEvent(scanTime, device->getID(), resp->rssi));
    addRecentDevice(device);

    processAdvert(device, scanTime, resp->data);
    processEpochs(device);
  }
  else
  {
    LOG_D("EbNRadioBT4", "Discovered non-EbN device (Address %s) [Checksum OK? %d] [Length OK? %d]", resp->address.toString().c_str(), addressOK, lengthOK);
  }
}

bool EbNRadioBT4::processAdvert(EbNDeviceBT4 *device, uint64_t time, const uint8_t *data)
{
  BitMap advert(31 * 8, data);
  size_t advertOffset = 0;

  // NOTE: Ignoring the version bit for now
  advertOffset += 1;

  uint32_t advertNum = 0;
  for(int b = 0; b < ADV_N_LOG2; b++)
  {
    if(advert.get(b + advertOffset))
    {
      advertNum |= (1 << b);
    }
  }
  advertOffset += ADV_N_LOG2;

  LOG_P("EbNRadioBT4", "Processing advert %u from device %d - '%s'", advertNum, device->getID(), advert.toHexString().c_str());

  if(advertNum < (RS_K + RS_M))
  {
    EbNDeviceBT4::Epoch *curEpoch = NULL;
    EbNDeviceBT4::Epoch *prevEpoch = NULL;
    bool isDuplicate = false;
    bool isNew = true;

    if(!device->epochs_.empty())
    {
      curEpoch = &device->epochs_.back();
      prevEpoch = &device->epochs_.front();
      if(prevEpoch == curEpoch)
      {
        prevEpoch = NULL;
      }

      uint64_t diffAdvertTime = time - curEpoch->lastAdvertTime;
      uint32_t diffAdvertNum = advertNum - curEpoch->lastAdvertNum;

      // This is a duplicate advertisement
      // Note that for SCAN_ACTIVE enabled, we adjust the comparison since we have to check
      // against either the advertisement or scan response as last received
      if((curEpoch->lastAdvertNum == advertNum) || (SCAN_ACTIVE && (curEpoch->lastAdvertNum == (advertNum + 1))))
      {
        if(diffAdvertTime < ((ADV_N / 2) * SCAN_INTERVAL))
        {
          isDuplicate = true;
          isNew = false;
        }
      }
      // This advertisement belongs to the current epoch
      else if((curEpoch->lastAdvertNum < advertNum) && (diffAdvertTime < (((diffAdvertNum + (SCAN_ACTIVE ? 6 : 3)) * SCAN_INTERVAL) / (SCAN_ACTIVE ? 2 : 1))))
      {
        isNew = false;
      }
    }

    // Skip processing any advertisement that we have already seen
    if(!isDuplicate)
    {
      if(isNew)
      {
        prevEpoch = curEpoch;

        device->epochs_.push_back(EbNDeviceBT4::Epoch(advertNum, time, dhCodeMatrix_, dhExchange_, (device->getAddress().getPartialValue(0x20) >> 5) & 0x1));
        curEpoch = &device->epochs_.back();

        LOG_P("EbNRadioBT4", "Creating new epoch, previous epoch %s", (prevEpoch == NULL) ? "does not exist" : "exists");
      }

      // Processing the DH symbol for the current epoch
      uint8_t dhSymbol[RS_W];
      advert.copyTo(dhSymbol, 0, advertOffset, 8 * RS_W);
      curEpoch->dhDecoder.setSymbol(advertNum, dhSymbol);
      advertOffset += 8 * RS_W;

      // Processing the DH symbol for the previous epoch if we are within the
      // first K-1 symbols in the epoch, and the previous epoch exists in a
      // non-decoded state
      if(advertNum < (RS_K - 1))
      {
        if((prevEpoch != NULL) && !prevEpoch->dhDecoder.isDecoded())
        {
          advert.copyTo(dhSymbol, 0, advertOffset, 8 * RS_W);
          prevEpoch->dhDecoder.setSymbol(advertNum + RS_K + RS_M, dhSymbol);
        }

        advertOffset += 8 * RS_W;
      }

      // Looking up the segmented Bloom filter (or creating a new one) to hold the
      // segment from this advertisement
      uint32_t bloomNum = advertNum / BF_B;
      SegmentedBloomFilter *bloom;

      if(!curEpoch->blooms.empty() && (curEpoch->blooms.back().first == bloomNum))
      {
        bloom = &curEpoch->blooms.back().second;
      }
      else
      {
        uint32_t totalSize = 0;
        vector<uint32_t> segmentSizes;
        for(int s = bloomNum * BF_B; s < (bloomNum + 1) * BF_B; s++)
        {
          uint32_t segmentSize = (s < (RS_K - 1)) ? (BF_SM - (8 * RS_W)) : BF_SM;
          segmentSizes.push_back(segmentSize);
          totalSize += segmentSize;
        }

        curEpoch->blooms.push_back(make_pair(bloomNum, SegmentedBloomFilter(BF_N, totalSize, BF_K, BF_B, segmentSizes, true)));
        bloom = &curEpoch->blooms.back().second;
      }

      // Copying the segment over into the Bloom filter
      bloom->setSegment(advertNum % BF_B, advert.toByteArray(), advertOffset);

      return true;
    }
  }

  return false;
}

void EbNRadioBT4::processEpochs(EbNDeviceBT4 *device)
{
  auto epochIt = device->epochs_.begin();
  while(epochIt != device->epochs_.end())
  {
    EbNDeviceBT4::Epoch &epoch = *epochIt;

    // Decoding the DH remote public value when possible
    if(!epoch.dhDecoder.isDecoded() && epoch.dhDecoder.canDecode())
    {
      const uint8_t *dhRemotePublic = epoch.dhDecoder.decode();
      epoch.decodeBloomNum = epoch.blooms.back().first;

      // Computing shared secret(s) from the DH exchange(s), only for non-active confirmation schemes
      if((confirmScheme_.type & ConfirmScheme::Active) != ConfirmScheme::Active)
      {
        for(auto dhIt = epoch.dhExchanges.begin(); dhIt != epoch.dhExchanges.end();  dhIt++)
        {
          ECDH &dhExchange = *dhIt;
          SharedSecret sharedSecret(confirmScheme_.type == ConfirmScheme::None);
          if(dhExchange.computeSharedSecret(sharedSecret, dhRemotePublic, epoch.dhExchangeYCoord))
          {
            device->addSharedSecret(sharedSecret);
          }
          else
          {
            LOG_E("EbNRadioBT4", "Could not compute shared secret for id %d", device->getID());
          }
        }
      }
    }

    // Processing all of the Bloom filters
    // TODO: Should store hashes on a per Bloom filter basis, so that there is
    // no overhead in checking individual segments as they come in
    if(epoch.dhDecoder.isDecoded() && !epoch.blooms.empty())
    {
      auto bloomIt = epoch.blooms.begin();
      while(bloomIt != epoch.blooms.end())
      {
        uint32_t bloomNum = bloomIt->first;
        SegmentedBloomFilter &bloom = bloomIt->second;

        // Only processing the Bloom filter if a new segment was added
        if(bloom.pFalse() != 1)
        {
          BitMap prefix(ADV_N_LOG2 + keySize_);
          for(int b = 0; b < ADV_N_LOG2; b++)
          {
            prefix.set(b, (bloomNum >> b) & 0x1);
          }
          prefix.copyFrom(epoch.dhDecoder.decode(), 0, ADV_N_LOG2, keySize_);

          // Updating the matching set, as well as shared secret confidence in the
          // case of passive or hybrid confirmation
          float bloomPFalse = bloom.resetPFalse();

          device->updateMatching(&bloom, prefix.toByteArray(), prefix.sizeBytes(), bloomPFalse);
          if(((confirmScheme_.type & ConfirmScheme::Passive) != 0) && (bloomNum > epoch.decodeBloomNum))
          {
            device->confirmPassive(&bloom, prefix.toByteArray(), prefix.sizeBytes(), confirmScheme_.threshold, bloomPFalse);
          }
        }

        // Removing any Bloom filters we are finished with (not the latest one,
        // or the last segment is filled)
        if((bloomNum != epoch.blooms.back().first) || bloom.isFilled(BF_B - 1))
        {
          bloomIt = epoch.blooms.erase(bloomIt);
        }
        else
        {
          bloomIt++;
        }
      }
    }

    // Removing any past epochs that we are finished with
    bool removed = false;
    if(distance(epochIt, device->epochs_.end()) > 1)
    {
      bool isDecoded = epoch.dhDecoder.isDecoded();
      bool isBeforePrevious = (device->epochs_.size() > 2);
      if(isDecoded || isBeforePrevious)
      {
        epochIt = device->epochs_.erase(epochIt);
        removed = true;

        LOG_D("EbNRadioBT4", "Finished with a prior epoch [Decoded? %d] [Before Previous? %d]", isDecoded, isBeforePrevious);
      }
    }

    if(!removed)
    {
      epochIt++;
    }
  }
}

vector<uint8_t> EbNRadioBT4::generateActiveHandshake(const ECDH &dhExchange)
{
  size_t messageOffset = 0;
  size_t messageSize = dhExchange.getPublicSize();
  if(ACTIVE_BF_ENABLED)
  {
    messageSize += (ACTIVE_BF_M + 7) / 8;
  }

  vector<uint8_t> message(messageSize, 0);

  // Adding the full public key (X and Y coordinates) to the message
  memcpy(message.data(), dhExchange.getPublic(), dhExchange.getPublicSize());
  messageOffset += dhExchange.getPublicSize();

  // Optionally add a Bloom filter to the message
  if(ACTIVE_BF_ENABLED)
  {
    BloomFilter bloom(BF_N, ACTIVE_BF_M, ACTIVE_BF_K);
    fillBloomFilter(&bloom, dhExchange.getPublic(), dhExchange.getPublicSize(), false);
    memcpy(message.data() + messageOffset, bloom.toByteArray(), bloom.sizeBytes());
    messageOffset += bloom.sizeBytes();
  }

  return message;
}

void EbNRadioBT4::processActiveHandshake(const vector<uint8_t> &message, EbNDeviceBT4 *device)
{
  // Note: We do not need to lock on dhExchange_ here since it can
  // only be modified by changeEpoch, which cannot be called while we
  // are handshaking.

  size_t messageOffset = 0;
  size_t messageSize = dhExchange_.getPublicSize();
  if(ACTIVE_BF_ENABLED)
  {
    messageSize += (ACTIVE_BF_M + 7) / 8;
  }

  if(message.size() != messageSize)
  {
    LOG_D("EbNRadioBT4", "Ignoring active handshake response - Invalid length");
    return;
  }

  // Computing the shared secret
  SharedSecret sharedSecret(SharedSecret::ConfirmScheme::Active);
  if(dhExchange_.computeSharedSecret(sharedSecret, message.data()))
  {
    device->addSharedSecret(sharedSecret);
  }
  else
  {
    LOG_E("EbNRadioBT4", "Could not compute shared secret for id %d", device->getID());
  }
  messageOffset += dhExchange_.getPublicSize();

  // Optionally update the matching set with the included Bloom filter
  if(ACTIVE_BF_ENABLED)
  {
    BloomFilter bloom(BF_N, ACTIVE_BF_M, ACTIVE_BF_K, message.data() + messageOffset);
    device->updateMatching(&bloom, message.data(), dhExchange_.getPublicSize());
    messageOffset += bloom.sizeBytes();
  }
}

void EbNRadioBT4::listen()
{
  while(true)
  {
    LOG_D("EbNRadioBT4", "Attempting to listen for incoming BT4 connections");

    int listenSock = hci_.listenBT4();

    LOG_D("EbNRadioBT4", "Waiting to accept incoming BT4 connection");

    pair<int, Address> clientInfo;
    while((clientInfo = hci_.acceptBT4(listenSock)).first != -1)
    {
      AndroidWake::grab("EbNListen");

      int clientSock = clientInfo.first;
      Address clientAddress = clientInfo.second;

      if(clientAddress.verifyChecksum())
      {
        LOG_D("EbNRadioBT4", "Accepted incoming connection from %s", clientAddress.toString().c_str());

        unique_lock<mutex> dhExchangeLock(dhExchangeMutex_);
        ECDH dhExchangeCopy(dhExchange_);
        dhExchangeLock.unlock();

        vector<uint8_t> localMessage = generateActiveHandshake(dhExchangeCopy);
        vector<uint8_t> remoteMessage(localMessage.size(), 0);

        if(hci_.recv(clientSock, remoteMessage.data(), remoteMessage.size(), 1000))
        {
          BitMap remoteMessageBM(dhExchangeCopy.getPublicSize() * 8, remoteMessage.data());
          LOG_D("EbNRadioBT4", "Received '%s' from remote device", remoteMessageBM.toHexString().c_str());

          bool allZeros = true;
          for(int b = 0; b < remoteMessage.size(); b++)
          {
            if(remoteMessage.data()[b] != 0)
            {
              allZeros = false;
              break;
            }
          }

          if(!allZeros && hci_.send(clientSock, localMessage.data(), localMessage.size(), 1000))
          {
            lock_guard<mutex> listenAdvertsLock(listenAdvertsMutex_);
            listenAdverts_.push_back(make_pair(clientAddress, remoteMessage));

            // Attempting to now wait for the connection to close on the other
            // side prior to closing it on this side, since we are sending last.
            // This should have the same timeout as a recv() call.
            LOG_D("EbNRadioBT4", "Waiting for remote client to close connection...");
            hci_.waitClose(clientSock, 1000);
            LOG_D("EbNRadioBT4", "Done!");
          }
        }
      }
      else
      {
        LOG_D("EbNRadioBT4", "Rejected incoming connection from address %s [Checksum OK? 0]", clientAddress.toString().c_str());
      }

      close(clientSock);

      AndroidWake::release("EbNListen");
    }

    close(listenSock);

    LOG_D("EbNRadioBT4", "Restarting the listen thread after 5 seconds...");
    sleepSec(5);
  }
}

size_t EbNRadioBT4::computeRSSymbolSize(size_t keySize, size_t advertBits)
{
  size_t bestW = 0;
  size_t bestNumAdverts = 100000;
  for(size_t w = 1; w < (keySize / 8); w++)
  {
    if((keySize / 8) % w != 0)
    {
      continue;
    }

    // This assumes knowledge of using BF_B = 2, BF_K = 1, and that the
    // threshold for passive confirmation is 95%.
    // TODO: Fix this ^
    size_t bloomBits = 2 * (advertBits - (8 * w));
    float bloomPFalse = BloomFilter::computePFalse(BF_N, bloomBits, 1);

    size_t advertsDecode = (keySize / 8) / w;
    size_t advertsConfirm = ceil(2 * (log(0.05) / log(bloomPFalse)));
    size_t advertsTotal = advertsDecode + advertsConfirm;

    if((advertsTotal < bestNumAdverts) || ((advertsTotal == bestNumAdverts) && (w < bestW)))
    {
      bestW = w;
      bestNumAdverts = advertsTotal;

      LOG_D("EbNRadioBT4", "Compute RS Symbol Size - New Best (W = %zu, #Advert = %zu)", bestW, bestNumAdverts);
    }
  }

  return bestW;
}

