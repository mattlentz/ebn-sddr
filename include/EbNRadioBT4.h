#ifndef EBNRADIOBT4_H
#define EBNRADIOBT4_H

#include <list>
#include <mutex>
#include <thread>
#include <vector>

#include "BluetoothHCI.h"
#include "CompileMath.h"
#include "EbNDeviceBT4.h"
#include "EbNDeviceMap.h"
#include "EbNRadio.h"
#include "ECDH.h"
#include "Logger.h"
#include "RSErasureEncoder.h"

class EbNRadioBT4 : public EbNRadio
{
public:
  static ConfirmScheme getDefaultConfirmScheme();

private:
  static const uint16_t ADVERT_MIN_INTERVAL = 650; // ms
  static const uint16_t ADVERT_MAX_INTERVAL = 700; // ms
  static const uint16_t SCAN_INTERVAL = 13500; // ms
  static const uint16_t SCAN_WINDOW = 1500; // ms
  static const bool SCAN_ACTIVE = true;

  static const size_t ADV_N = (SCAN_ACTIVE ? 2 : 1) * ((EPOCH_INTERVAL + (SCAN_INTERVAL - 1)) / SCAN_INTERVAL);
  static const size_t ADV_N_LOG2 = CLog<ADV_N>::value;

  static const size_t ACTIVE_BF_M = 1108;
  static const size_t ACTIVE_BF_K = 3;
  static const bool ACTIVE_BF_ENABLED = true;

private:
  const size_t RS_W;
  const size_t RS_K;
  const size_t RS_M;
  const size_t BF_SM;
  const size_t BF_K;
  const size_t BF_B;

  BluetoothHCI hci_;
  EbNDeviceMap<EbNDeviceBT4> deviceMap_;
  RSMatrix dhCodeMatrix_;
  RSErasureEncoder dhEncoder_;
  std::vector<uint8_t> dhPrevSymbols_;
  ECDH dhExchange_;
  std::mutex dhExchangeMutex_;
  size_t advertNum_;
  SegmentedBloomFilter advertBloom_;
  size_t advertBloomNum_;
  std::thread listenThread_;
  std::list<std::pair<Address, std::vector<uint8_t> > > listenAdverts_;
  std::mutex listenAdvertsMutex_;

public:
  EbNRadioBT4(size_t keySize, ConfirmScheme confirmScheme, MemoryScheme memoryScheme, int adapterID);

  // EbNRadio interface
  void initialize();
  std::list<DiscoverEvent> discover();
  void changeEpoch();
  std::set<DeviceID> handshake(const std::set<DeviceID> &deviceIDs);
  EncounterEvent doneWithDevice(DeviceID id);

  BitMap generateAdvert(size_t advertNum);
  bool processAdvert(EbNDeviceBT4 *device, uint64_t time, const uint8_t *data);
  void processEpochs(EbNDeviceBT4 *device);

private:
  BluetoothHCI::UndirectedAdvert getAdvertType(bool canAllowConnections = true);

  void changeAdvert();
  void processScanResponse(std::list<DiscoverEvent> *discovered, const ScanResponse *resp);

  std::vector<uint8_t> generateActiveHandshake(const ECDH &dhExchange);
  void processActiveHandshake(const std::vector<uint8_t> &message, EbNDeviceBT4 *device);

  void listen();

private:
  static size_t computeRSSymbolSize(size_t keySize, size_t advertBits);
};

#endif // EBNRADIOBT4_H

