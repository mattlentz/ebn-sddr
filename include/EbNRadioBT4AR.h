#ifndef EBNRADIOBT4AR_H
#define EBNRADIOBT4AR_H

#include <list>

#include "BluetoothHCI.h"
#include "EbNDeviceBT4AR.h"
#include "EbNDeviceMap.h"
#include "EbNRadio.h"
#include "ECDH.h"
#include "Logger.h"

class EbNRadioBT4AR : public EbNRadio
{
public:
  static ConfirmScheme getDefaultConfirmScheme();

private:
  static const uint16_t ADVERT_MIN_INTERVAL = 650; // ms
  static const uint16_t ADVERT_MAX_INTERVAL = 700; // ms
  static const uint32_t SCAN_INTERVAL = TIME_SEC_TO_MS(60); // ms
  static const uint16_t SCAN_WINDOW = 1500; // ms

private:
  BluetoothHCI hci_;
  EbNDeviceMap<EbNDeviceBT4AR> deviceMap_;
  std::vector<uint8_t> keyIRK_;

public:
  EbNRadioBT4AR(size_t keySize, ConfirmScheme confirmScheme, MemoryScheme memoryScheme, int adapterID);

  // EbNRadio interface
  void initialize();
  std::list<DiscoverEvent> discover();
  void changeEpoch();
  std::set<DeviceID> handshake(const std::set<DeviceID> &deviceIDs);
  EncounterEvent doneWithDevice(DeviceID id);

private:
  void processScanResponse(std::list<DiscoverEvent> *discovered, const ScanResponse *resp);
  bool processAdvert(EbNDeviceBT4AR *device, uint64_t time, const uint8_t *data);
};

#endif // EBNRADIOBT4AR_H


