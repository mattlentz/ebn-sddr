#include "EbNDeviceBT4.h"

using namespace std;

EbNDeviceBT4::EbNDeviceBT4(DeviceID id, const Address &address, const LinkValueList &listenSet)
   : EbNDevice(id, address, listenSet),
     epochs_()
{
}

EbNDeviceBT4::Epoch::Epoch(uint32_t advertNum, uint64_t advertTime, const RSMatrix &dhCodeMatrix, const ECDH &dhExchange, bool dhExchangeYCoord)
   : lastAdvertNum(advertNum),
     lastAdvertTime(advertTime),
     dhDecoder(dhCodeMatrix),
     dhExchanges(1, dhExchange),
     dhExchangeYCoord(dhExchangeYCoord),
     blooms(),
     decodeBloomNum(0) 
{
}

