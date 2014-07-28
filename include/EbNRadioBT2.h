#ifndef EBNRADIOBT2_H
#define EBNRADIOBT2_H

#include <list>
#include <thread>
#include <unordered_set>

#include "BluetoothHCI.h"
#include "CompileMath.h"
#include "EbNDeviceBT2.h"
#include "EbNDeviceMap.h"
#include "EbNRadio.h"
#include "ECDH.h"
#include "Logger.h"

class EbNRadioBT2 : public EbNRadio
{
public:
  static ConfirmScheme getDefaultConfirmScheme();

private:
  static const uint32_t DISC_INTERVAL = TIME_SEC_TO_MS(60); // ms
  static const uint32_t DISC_PERIODS = 8;
  static const uint32_t ADV_N = (EPOCH_INTERVAL + (DISC_INTERVAL - 1)) / DISC_INTERVAL;
  static const uint32_t ADV_N_LOG2 = CLog<ADV_N>::value;

private:
  const uint32_t BF_M;
  const uint32_t BF_K;

  BluetoothHCI hci_;
  EbNDeviceMap<EbNDeviceBT2> deviceMap_;
  ECDH dhExchange_;
  uint32_t advertNum_;
  std::thread listenThread_;

public:
  EbNRadioBT2(size_t keySize, ConfirmScheme confirmScheme, MemoryScheme memoryScheme, int adapterID);

  // EbNRadio interface
  void initialize();
  std::list<DiscoverEvent> discover();
  void changeEpoch();
  std::set<DeviceID> handshake(const std::set<DeviceID> &deviceIDs);
  EncounterEvent doneWithDevice(DeviceID id);

  BitMap generateAdvert(size_t advertNum);
  bool processAdvert(EbNDeviceBT2 *device, uint64_t time, const uint8_t *data, bool computeSecret = true);
  void processEpochs(EbNDeviceBT2 *device);

private:
  void changeAdvert();
  void processEIRResponse(std::list<DiscoverEvent> *discovered, const EIRInquiryResponse *response);

  void listen();
};

#endif  // EBNRADIOBT2_H

