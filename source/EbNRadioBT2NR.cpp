#include "EbNRadioBT2NR.h"

#include <future>
#include <stdexcept>

#include "BinaryToUTF8.h"

using namespace std;

EbNRadio::ConfirmScheme EbNRadioBT2NR::getDefaultConfirmScheme()
{
  return {ConfirmScheme::None, 0};
}

EbNRadioBT2NR::EbNRadioBT2NR(size_t keySize, ConfirmScheme confirmScheme, MemoryScheme memoryScheme, int adapterID)
   : EbNRadio(keySize, confirmScheme, memoryScheme),
     BF_M(NAME_DECODED_SIZE - 2 - keySize),
     BF_K(3),
     hci_(adapterID),
     deviceMap_(),
     dhExchange_(keySize),
     listenThread_()
{
  if(confirmScheme.type != ConfirmScheme::None)
  {
    throw std::runtime_error("Invalid Confirmation Scheme for EbNRadioBT2NR: Only supports 'None'.");
  }
}

void EbNRadioBT2NR::initialize()
{
  // Disabling discoverable and EIR settings so that we can set up the new
  // address and first payload before remote devices can receive it
  hci_.setDiscoverable(false);

  hci_.setPublicAddress(Address::generate(6));

  changeAdvert();

  hci_.setInquiryMode(BluetoothHCI::InquiryMode::Standard);
  hci_.setDiscoverable(true);
  hci_.setConnectable(true);
}

list<DiscoverEvent> EbNRadioBT2NR::discover()
{
  if(memoryScheme_ == MemoryScheme::NoMemory)
  {
    deviceMap_.clear();
  }

  uint64_t curTime = getTimeMS();

  changeAdvert();

  list<InquiryResponse> responses = hci_.performInquiry(DISC_PERIODS);
  list<DiscoverEvent> discovered;
  for(auto respIt = responses.cbegin(); respIt != responses.cend(); respIt++)
  {
    const InquiryResponse &resp = *respIt;

    EbNDeviceBT2 *device = deviceMap_.get(resp.address);
    if(device == NULL)
    {
      lock_guard<mutex> setLock(setMutex_);

      device = new EbNDeviceBT2(generateDeviceID(), resp.address, resp.clockOffset, resp.pageScanMode, listenSet_);
      deviceMap_.add(resp.address, device);

      LOG_P("EbNRadioBT2NR", "Discovered new device (ID %d, Address %s)", device->getID(), device->getAddress().toString().c_str());
    }

    discovered.push_back(DiscoverEvent(curTime, device->getID(), 0));
    addRecentDevice(device);
  }

  nextDiscover_ += DISC_INTERVAL - 1000 + (rand() % 2001);

  return discovered;
}

void EbNRadioBT2NR::changeEpoch()
{
  // Generate a new secret for this epoch's DH exchanges
  dhExchange_.generateSecret();

  hci_.setPublicAddress(hci_.getPublicAddress().shift());

  hci_.setDiscoverable(false);
  changeAdvert();
  hci_.setInquiryMode(BluetoothHCI::InquiryMode::Standard);
  hci_.setDiscoverable(true);

  nextChangeEpoch_ += EPOCH_INTERVAL;
}

set<DeviceID> EbNRadioBT2NR::handshake(const set<DeviceID> &deviceIDs)
{
  set<DeviceID> encountered;

  for(auto it = deviceIDs.begin(); it != deviceIDs.end(); it++)
  {
    EbNDeviceBT2 *device = deviceMap_.get(*it);

    // Making sure that we have enough time until our next action
    ActionInfo nextAction = getNextAction();
    if(nextAction.timeUntil < 2000)
    {
      break;
    }

    string remoteName;
    bool readOK = hci_.readRemoteName(remoteName, device->getAddress(), device->clockOffset_, device->pageScanMode_, 2500);
    bool lengthOK = (BinaryToUTF8::getNumEncodedBits(NAME_DECODED_SIZE) == (remoteName.size() * 8));
    if(readOK && lengthOK)
    {
      BitMap advert(NAME_DECODED_SIZE);
      BinaryToUTF8::decode(advert.toByteArray(), remoteName);

      LOG_P("EbNRadioBT2NR", "Processing advert from device %d - '%s'", device->getID(), advert.toHexString().c_str());

      // NOTE: Ignoring version bit for now
      size_t advertOffset = 1;

      // Computing the shared secret
      bool remotePublicY = advert.get(advertOffset);
      advertOffset += 1;

      vector<uint8_t> remotePublicX(keySize_ / 8, 0);
      advert.copyTo(remotePublicX.data(), 0, advertOffset, keySize_);
      advertOffset += keySize_;

      SharedSecret sharedSecret(confirmScheme_.type == ConfirmScheme::None);
      if(dhExchange_.computeSharedSecret(sharedSecret, remotePublicX.data(), remotePublicY))
      {
        device->addSharedSecret(sharedSecret);
      }
      else
      {
        LOG_E("EbNRadioBT2NR", "Could not compute shared secret for id %d", device->getID());
      }

      // Updating the matching set based on the Bloom filter
      BloomFilter bloom(BF_N, BF_K, advert, advertOffset, BF_M);
      device->updateMatching(&bloom, remotePublicX.data(), keySize_ / 8);
    }
    else if(readOK)
    {
      LOG_D("EbNRadioBT2NR", "Ignoring non-EbN device id %d [Length OK? %d]", device->getID(), lengthOK);
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

EncounterEvent EbNRadioBT2NR::doneWithDevice(DeviceID id)
{
  EbNDeviceBT2 *device = deviceMap_.get(id);

  EncounterEvent expiredEvent(getTimeMS());
  device->getEncounterInfo(expiredEvent, true);

  deviceMap_.remove(id);
  removeRecentDevice(id);

  return expiredEvent;
}

void EbNRadioBT2NR::changeAdvert()
{
  BitMap advert(NAME_DECODED_SIZE);
  size_t advertOffset = 0;

  // NOTE: Version bit is 0, since we are not an infrastructure node
  advertOffset += 1;

  // Insert the current DH exchange public key
  advert.set(advertOffset, dhExchange_.getPublicY());
  advertOffset += 1;

  advert.copyFrom(dhExchange_.getPublicX(), 0, advertOffset, keySize_);
  advertOffset += keySize_;

  BloomFilter advertBloom(BF_N, BF_M, BF_K);
  fillBloomFilter(&advertBloom, dhExchange_.getPublicX(), keySize_ / 8);
  advert.copyFrom(advertBloom.toByteArray(), 0, advertOffset, advertBloom.M());

  // Setting the advertisement data to respond to name requests
  hci_.writeLocalName(BinaryToUTF8::encode(advert.toByteArray(), advert.size()));
  LOG_P("EbNRadioBT2NR", "Setting advert data to \'%s\'", advert.toHexString().c_str());
}

void EbNRadioBT2NR::listen()
{
  LOG_E("EbNRadioBT2NR", "Implement active handshake listen loop at %s:%d", __FILE__, __LINE__);
}

