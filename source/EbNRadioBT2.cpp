#include "EbNRadioBT2.h"

#include "AndroidWake.h"

#include <future>

using namespace std;

EbNRadio::ConfirmScheme EbNRadioBT2::getDefaultConfirmScheme()
{
  return {ConfirmScheme::Passive, 0.05};
}

EbNRadioBT2::EbNRadioBT2(size_t keySize, ConfirmScheme confirmScheme, MemoryScheme memoryScheme, int adapterID)
   : EbNRadio(keySize, confirmScheme, memoryScheme),
     BF_M((238 * 8) - 1 - ADV_N_LOG2 - keySize),
     BF_K(4),
     hci_(adapterID),
     deviceMap_(),
     dhExchange_(keySize),
     advertNum_(0),
     listenThread_()
{
}

void EbNRadioBT2::initialize()
{
  // Disabling discoverable and EIR settings so that we can set up the new
  // address and first payload before remote devices can receive it
  hci_.setDiscoverable(false);

  uint8_t partial = (uint8_t)dhExchange_.getPublicY() << 5;
  hci_.setPublicAddress(Address::generateWithPartial(6, partial, 0x20));

  changeAdvert();

  hci_.setInquiryMode(BluetoothHCI::InquiryMode::WithRSSIAndEIR);
  hci_.setDiscoverable(true);
  hci_.setConnectable(true);

  // Start a thread to listen for incoming connections in the case of active or
  // hybrid confirmation schemes
  if((confirmScheme_.type & ConfirmScheme::Active) != 0)
  {
    listenThread_ = thread(&EbNRadioBT2::listen, this);
  }
}

list<DiscoverEvent> EbNRadioBT2::discover()
{
  if(memoryScheme_ == MemoryScheme::NoMemory)
  {
    deviceMap_.clear();
  }

  changeAdvert();

  list<DiscoverEvent> discovered;
  function<void(const EIRInquiryResponse *)> callback = bind(&EbNRadioBT2::processEIRResponse, this, &discovered, placeholders::_1);
  hci_.performEIRInquiry(callback, DISC_PERIODS);

  nextDiscover_ += DISC_INTERVAL - 1000 + (rand() % 2001);

  return discovered;
}

void EbNRadioBT2::changeEpoch()
{
  advertNum_ = 0;

  // Generate a new secret for this epoch's DH exchanges
  dhExchange_.generateSecret();

  // Shifting the current address. Uses the 1 bit Y coordinate as part of the
  // address since compressed ECDH keys are actually (keySize + 1) bits long
  uint8_t partial = (uint8_t)dhExchange_.getPublicY() << 5;
  hci_.setPublicAddress(hci_.getPublicAddress().shiftWithPartial(partial, 0x20));

  hci_.setDiscoverable(false);
  changeAdvert();
  hci_.setInquiryMode(BluetoothHCI::InquiryMode::WithRSSIAndEIR);
  hci_.setDiscoverable(true);

  // Computing new shared secrets in the case of passive or hybrid confirmation
  if((confirmScheme_.type & ConfirmScheme::Passive) != 0)
  {
    for(auto it = deviceMap_.begin(); it != deviceMap_.end(); it++)
    {
      EbNDeviceBT2 *device = it->second;
      if(!device->epochs_.empty())
      {
        EbNDeviceBT2::Epoch &curEpoch = device->epochs_.back();

        SharedSecret sharedSecret(confirmScheme_.type == ConfirmScheme::None);
        if(dhExchange_.computeSharedSecret(sharedSecret, curEpoch.dhRemotePublic.data()))
        {
          device->addSharedSecret(sharedSecret);
        }
        else
        {
          LOG_E("EbNRadioBT2", "Could not compute shared secret for id %d", device->getID());
        }
      }
    }
  }

  nextChangeEpoch_ += EPOCH_INTERVAL;
}

set<DeviceID> EbNRadioBT2::handshake(const set<DeviceID> &deviceIDs)
{
  set<DeviceID> encountered;

  for(auto it = deviceIDs.begin(); it != deviceIDs.end(); it++)
  {
    EbNDeviceBT2 *device = deviceMap_.get(*it);

    // Only perform active confirmation once per device
    if(!device->isConfirmed() && (getHandshakeScheme() == ConfirmScheme::Active))
    {
      LOG_E("EbNRadioBT2", "TODO: Implement active handshake at %s:%d", __FILE__, __LINE__);
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

EncounterEvent EbNRadioBT2::doneWithDevice(DeviceID id)
{
  EbNDeviceBT2 *device = deviceMap_.get(id);

  EncounterEvent expiredEvent(getTimeMS());
  device->getEncounterInfo(expiredEvent, true);

  deviceMap_.remove(id);
  removeRecentDevice(id);

  return expiredEvent;
}

void EbNRadioBT2::changeAdvert()
{
  if(advertNum_ >= ADV_N)
  {
    LOG_D("EbNRadioBT2", "Reached last unique advert, waiting for epoch change\n");
    return;
  }

  BitMap advert = generateAdvert(advertNum_);

  // Setting the advertisement data to periodically broadcast
  hci_.writeExtInquiryResponse(advert.toByteArray());
  LOG_P("EbNRadioBT2", "Setting advert data to \'%s\'", advert.toHexString().c_str());

  advertNum_++;
}

void EbNRadioBT2::processEIRResponse(list<DiscoverEvent> *discovered, const EIRInquiryResponse *resp)
{
  uint64_t scanTime = getTimeMS();

  bool addressOK = resp->address.verifyChecksum();
  bool lengthOK = (resp->data[0] == 239);
  bool typeOK = (resp->data[1] == 0xFF);
  if(addressOK && lengthOK && typeOK)
  {
    EbNDeviceBT2 *device = deviceMap_.get(resp->address);
    if(device == NULL)
    {
      lock_guard<mutex> setLock(setMutex_);

      device = new EbNDeviceBT2(generateDeviceID(), resp->address, resp->clockOffset, resp->pageScanMode, listenSet_);
      deviceMap_.add(resp->address, device);

      LOG_P("EbNRadioBT2", "Discovered new EbN device (ID %d, Address %s)", device->getID(), device->getAddress().toString().c_str());
    }

    device->addRSSIMeasurement(scanTime, resp->rssi);
    discovered->push_back(DiscoverEvent(scanTime, device->getID(), resp->rssi));
    addRecentDevice(device);

    // Processing the advertisement in the case of passive or hybrid
    // confirmation, regardless of the state of the stable device detector
    if((confirmScheme_.type == ConfirmScheme::None) || ((confirmScheme_.type & ConfirmScheme::Passive) != 0))
    {
      processAdvert(device, scanTime, resp->data);
      processEpochs(device);
    }
  }
  else
  {
    LOG_D("EbNRadioBT2", "Discovered non-EbN device (Address %s) [Checksum OK? %d] [Length OK? %d] [Type OK? %d]", resp->address.toString().c_str(), addressOK, lengthOK, typeOK);
  }
}

BitMap EbNRadioBT2::generateAdvert(size_t advertNum)
{
  BitMap advert(240 * 8);
  size_t advertOffset = 0;

  advert.setAll(false);
  advert.setByte(0, 239);
  advert.setByte(1, 0xFF);
  advertOffset += 16;

  // NOTE: Version bit is 0, since we are not an infrastructure node
  advertOffset += 1;

  // Inserting the advertisement number as the first portion of the advert
  for(int b = 0; b < ADV_N_LOG2; b++)
  {
    advert.set(b + advertOffset, (advertNum >> b) & 0x1);
  }
  advertOffset += ADV_N_LOG2;

  // Insert the current DH exchange public key
  advert.copyFrom(dhExchange_.getPublicX(), 0, advertOffset, keySize_);
  advertOffset += keySize_;

  // Computing a new Bloom filter
  BitMap prefix(ADV_N_LOG2 + keySize_);
  for(int b = 0; b < ADV_N_LOG2; b++)
  {
    prefix.set(b, (advertNum >> b) & 0x1);
  }
  prefix.copyFrom(dhExchange_.getPublicX(), 0, ADV_N_LOG2, keySize_);

  BloomFilter advertBloom(BF_N, BF_M, BF_K);
  fillBloomFilter(&advertBloom, prefix.toByteArray(), prefix.sizeBytes());
  advert.copyFrom(advertBloom.toByteArray(), 0, advertOffset, advertBloom.M());

  return advert;
}

bool EbNRadioBT2::processAdvert(EbNDeviceBT2 *device, uint64_t time, const uint8_t *data, bool computeSecret)
{
  BitMap advert(240 * 8, data);
  size_t advertOffset = 17;

  uint32_t advertNum = 0;
  for(int b = 0; b < ADV_N_LOG2; b++)
  {
    if(advert.get(b + advertOffset))
    {
      advertNum |= (1 << b);
    }
  }
  advertOffset += ADV_N_LOG2;

  LOG_D("EbNRadioBT2", "Processing advertisement %u for device %d", advertNum, device->getID());

  if(advertNum < ADV_N)
  {
    EbNDeviceBT2::Epoch *curEpoch;
    bool isDuplicate = false;
    bool isNew = true;

    if(!device->epochs_.empty())
    {
      curEpoch = &device->epochs_.back();

      uint64_t diffAdvertTime = time - curEpoch->lastAdvertTime;
      uint32_t diffAdvertNum = advertNum - curEpoch->lastAdvertNum;

      // This is a duplicate advertisement
      if((curEpoch->lastAdvertNum == advertNum))
      {
        if(diffAdvertTime < ((ADV_N - 3) * DISC_INTERVAL))
        {
          isDuplicate = true;
          isNew = false;
        }
      }
      // This advertisement belongs to the current epoch
      else if((curEpoch->lastAdvertNum < advertNum) && (diffAdvertTime < ((diffAdvertNum + 3) * DISC_INTERVAL)))
      {
        isNew = false;
      }
    }

    // Skip processing any advertisement that we have already seen
    if(!isDuplicate)
    {
      if(isNew)
      {
        device->epochs_.push_back(EbNDeviceBT2::Epoch(advertNum, time, keySize_));
        curEpoch = &device->epochs_.back();

        advert.copyTo(curEpoch->dhRemotePublic.data(), 0, advertOffset, keySize_);

        if(computeSecret)
        {
          SharedSecret sharedSecret(confirmScheme_.type == ConfirmScheme::None);
          if(dhExchange_.computeSharedSecret(sharedSecret, curEpoch->dhRemotePublic.data(), device->getAddress().getPartialValue(0x20) >> 5))
          {
            device->addSharedSecret(sharedSecret);
          }
          else
          {
            LOG_E("EbNRadioBT2", "Could not compute shared secret for id %d", device->getID());
          }
        }
      }

      // Updating the matching set, as well as shared secret confidence in the
      // case of passive or hybrid confirmation, based on the Bloom filter
      // contained in the advertisement
      BloomFilter bloom(BF_N, BF_K, advert, advertOffset, advert.size() - advertOffset);

      BitMap prefix(ADV_N_LOG2 + keySize_);
      for(int b = 0; b < ADV_N_LOG2; b++)
      {
        prefix.set(b, (advertNum >> b) & 0x1);
      }
      prefix.copyFrom(curEpoch->dhRemotePublic.data(), 0, ADV_N_LOG2, keySize_);

      device->updateMatching(&bloom, prefix.toByteArray(), prefix.sizeBytes());
      if(((confirmScheme_.type & ConfirmScheme::Passive) != 0) && !isNew)
      {
        device->confirmPassive(&bloom, prefix.toByteArray(), prefix.sizeBytes(), confirmScheme_.threshold);
      }

      return true;
    }
  }

  return false;
}

void EbNRadioBT2::processEpochs(EbNDeviceBT2 *device)
{
  auto epochIt = device->epochs_.begin();
  while(epochIt != device->epochs_.end())
  {
    // Removing any past epochs that we are finished with
    if(device->epochs_.size() > 1)
    {
      epochIt = device->epochs_.erase(epochIt);
    }
    else
    {
      epochIt++;
    }
  }
}

void EbNRadioBT2::listen()
{
  LOG_E("EbNRadioBT2", "TODO: Implement active handshake listen loop at %s:%d", __FILE__, __LINE__);
}

