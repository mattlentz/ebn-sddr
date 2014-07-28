#include "EbNRadioBT4AR.h"

#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

using namespace std;

EbNRadio::ConfirmScheme EbNRadioBT4AR::getDefaultConfirmScheme()
{
  return {ConfirmScheme::None, 0};
}

EbNRadioBT4AR::EbNRadioBT4AR(size_t keySize, ConfirmScheme confirmScheme, MemoryScheme memoryScheme, int adapterID)
   : EbNRadio(keySize, confirmScheme, memoryScheme),
     hci_(adapterID),
     deviceMap_(),
     keyIRK_(16, 0)
{
  if(confirmScheme.type != ConfirmScheme::None)
  {
    throw std::runtime_error("Invalid Confirmation Scheme for EbNRadioBT4AR: Only supports 'None'.");
  }
}

void EbNRadioBT4AR::initialize()
{
  hci_.setDiscoverable(false);
  hci_.setConnectable(false);

  // Disabling advertising so that we can set up the new address and first
  // advertisement before remote devices can receive it
  hci_.enableAdvertising(false);

  hci_.setRandomAddress(Address::generate(6));

  hci_.setUndirectedAdvertParams(BluetoothHCI::UndirectedAdvert::Standard, BluetoothHCI::AdvertFilter::ScanAllConnectAll, ADVERT_MIN_INTERVAL, ADVERT_MAX_INTERVAL);
  changeEpoch();
  nextChangeEpoch_ -= EPOCH_INTERVAL;

  hci_.enableAdvertising(true);
}

list<DiscoverEvent> EbNRadioBT4AR::discover()
{
  if(memoryScheme_ == MemoryScheme::NoMemory)
  {
    deviceMap_.clear();
  }

  list<DiscoverEvent> discovered;
  function<void(const ScanResponse *)> callback = bind(&EbNRadioBT4AR::processScanResponse, this, &discovered, placeholders::_1);
  hci_.performScan(BluetoothHCI::Scan::Passive, BluetoothHCI::DuplicateFilter::On, SCAN_WINDOW, callback);

  nextDiscover_ += SCAN_INTERVAL;

  return discovered;
}

void EbNRadioBT4AR::changeEpoch()
{
  Address nextAddress = Address::generate(6);

  uint8_t output[16];
  int updateSize = sizeof(output);
  int outputSize = sizeof(output);

  EVP_CIPHER_CTX evpContext;
  EVP_CIPHER_CTX_init(&evpContext);
  EVP_EncryptInit_ex(&evpContext, EVP_aes_128_ecb(), NULL, keyIRK_.data(), NULL);
  EVP_EncryptUpdate(&evpContext, output, &updateSize, nextAddress.toByteArray() + 3, 3);
  EVP_EncryptFinal_ex(&evpContext, output, &outputSize);
  EVP_CIPHER_CTX_cleanup(&evpContext);

  memcpy(nextAddress.toByteArray(), output + 13, 3);

  hci_.enableAdvertising(false);
  hci_.setRandomAddress(nextAddress);
  hci_.enableAdvertising(true);

  // Setting the next time to change epochs
  nextChangeEpoch_ += EPOCH_INTERVAL;
}

set<DeviceID> EbNRadioBT4AR::handshake(const set<DeviceID> &deviceIDs)
{
  set<DeviceID> encountered;

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

EncounterEvent EbNRadioBT4AR::doneWithDevice(DeviceID id)
{
  EbNDeviceBT4AR *device = deviceMap_.get(id);

  EncounterEvent expiredEvent(getTimeMS());
  device->getEncounterInfo(expiredEvent, true);

  deviceMap_.remove(id);
  removeRecentDevice(id);

  return expiredEvent;
}

void EbNRadioBT4AR::processScanResponse(list<DiscoverEvent> *discovered, const ScanResponse *resp)
{
  uint64_t scanTime = getTimeMS();

  EbNDeviceBT4AR *device = deviceMap_.get(resp->address);
  if(device == NULL)
  {
    device = new EbNDeviceBT4AR(generateDeviceID(), resp->address, listenSet_);
    deviceMap_.add(resp->address, device);

    LOG_P("EbNRadioBT4AR", "Discovered new EbN device (ID %d, Address %s)", device->getID(), device->getAddress().toString().c_str());
  }

  discovered->push_back(DiscoverEvent(scanTime, device->getID(), resp->rssi));
  addRecentDevice(device);
}

bool EbNRadioBT4AR::processAdvert(EbNDeviceBT4AR *device, uint64_t time, const uint8_t *data)
{
}

