#ifndef EBNDEVICEBT4_H
#define EBNDEVICEBT4_H

#include <cstdint>
#include <list>

#include "EbNDevice.h"
#include "ECDH.h"
#include "SegmentedBloomFilter.h"
#include "RSErasureDecoder.h"

class EbNRadioBT4;

class EbNDeviceBT4 : public EbNDevice
{
  friend EbNRadioBT4;

private:
  typedef std::list<std::pair<size_t, SegmentedBloomFilter> > BloomList;

  struct Epoch
  {
    uint32_t lastAdvertNum;
    uint64_t lastAdvertTime;
    RSErasureDecoder dhDecoder;
    std::list<ECDH> dhExchanges;
    bool dhExchangeYCoord;
    BloomList blooms;
    uint32_t decodeBloomNum;

    Epoch(uint32_t advertNum, uint64_t advertTime, const RSMatrix &dhCodeMatrix, const ECDH &dhExchange, bool dhExchangeYCoord);
  };

private:
  std::list<Epoch> epochs_;

public:
  EbNDeviceBT4(DeviceID id, const Address &address, const LinkValueList &listenSet);
};

#endif // EBNDEVICEBT4_H

