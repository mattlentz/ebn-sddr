#ifndef EBNRADIOBT2PSI_H
#define EBNRADIOBT2PSI_H

#include <list>
#include <thread>
#include <unordered_set>

#include "jl10psi/base.h"
#include "jl10psi/config.h"
#include "jl10psi/jl10_psi_client.h"
#include "jl10psi/jl10_psi_server.h"
#include "jl10psi/basic_tool.h"

#include "BluetoothHCI.h"
#include "CompileMath.h"
#include "EbNDeviceBT2.h"
#include "EbNDeviceMap.h"
#include "EbNRadio.h"
#include "ECDH.h"
#include "Logger.h"

class EbNRadioBT2PSI : public EbNRadio
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

  vector<string> psiClientData_;
  vector<string> psiServerData_;
  std::unordered_set<Address, Address::Hash, Address::Equal> discDevices_;
  std::mutex discDevicesMutex_;

public:
  EbNRadioBT2PSI(size_t keySize, ConfirmScheme confirmScheme, MemoryScheme memoryScheme, int adapterID);

  // EbNRadio interface
  void setAdvertisedSet(const LinkValueList &advertisedSet);
  void setListenSet(const LinkValueList &listenSet);
  void initialize();
  std::list<DiscoverEvent> discover();
  void changeEpoch();
  std::set<DeviceID> handshake(const std::set<DeviceID> &deviceIDs);
  EncounterEvent doneWithDevice(DeviceID id);

private:
  void processEIRResponse(std::list<DiscoverEvent> *discovered, const EIRInquiryResponse *response);

  void listen();

  void constructPSIMessage(std::vector<uint8_t> &message, std::vector<string> input);
  std::vector<string> parsePSIMessage(const std::vector<uint8_t> &message);
};

#endif  // EBNRADIOBT2PSI_H

