#ifndef EBNRADIOBT2NR_H
#define EBNRADIOBT2NR_H

#include <list>
#include <thread>

#include "BluetoothHCI.h"
#include "CompileMath.h"
#include "EbNDeviceBT2.h"
#include "EbNDeviceMap.h"
#include "EbNRadio.h"
#include "ECDH.h"
#include "Logger.h"

class EbNRadioBT2NR : public EbNRadio
{
public:
  static ConfirmScheme getDefaultConfirmScheme();

private:
  static const uint32_t DISC_INTERVAL = TIME_SEC_TO_MS(60); // ms
  static const uint32_t DISC_PERIODS = 8;
  static const uint32_t NAME_DECODED_SIZE = 1723;

private:
  const uint32_t BF_M;
  const uint32_t BF_K;

  BluetoothHCI hci_;
  EbNDeviceMap<EbNDeviceBT2> deviceMap_;
  ECDH dhExchange_;
  std::thread listenThread_;

public:
  EbNRadioBT2NR(size_t keySize, ConfirmScheme confirmScheme, MemoryScheme memoryScheme, int adapterID);

  // EbNRadio interface
  void initialize();
  std::list<DiscoverEvent> discover();
  void changeEpoch();
  std::set<DeviceID> handshake(const std::set<DeviceID> &deviceIDs);
  EncounterEvent doneWithDevice(DeviceID id);

private:
  void changeAdvert();

  void listen();
};

#endif  // EBNRADIOBT2NR_H

