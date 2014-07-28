#ifndef EBNDEVICEBT2_H
#define EBNDEVICEBT2_H

#include <cstdint>
#include <list>
#include <vector>

#include "BloomFilter.h"
#include "EbNDevice.h"
#include "ECDH.h"

class EbNRadioBT2;
class EbNRadioBT2NR;
class EbNRadioBT2PSI;

class EbNDeviceBT2 : public EbNDevice
{
  friend EbNRadioBT2;
  friend EbNRadioBT2NR;
  friend EbNRadioBT2PSI;

private:
  struct Epoch
  {
    uint32_t lastAdvertNum;
    uint64_t lastAdvertTime;
    std::vector<uint8_t> dhRemotePublic;

    Epoch(uint32_t advertNum, uint64_t advertTime, size_t keySize);
  };

private:
  uint16_t clockOffset_;
  uint8_t pageScanMode_;
  std::list<Epoch> epochs_;

public:
  EbNDeviceBT2(DeviceID id, const Address &address, uint16_t clockOffset, uint8_t pageScanMode, const LinkValueList &listenSet);
};

#endif // EBNDEVICEBT2_H

