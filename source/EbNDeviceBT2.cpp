#include "EbNDeviceBT2.h"

EbNDeviceBT2::EbNDeviceBT2(DeviceID id, const Address &address, uint16_t clockOffset, uint8_t pageScanMode, const LinkValueList &listenSet)
   : EbNDevice(id, address, listenSet),
     clockOffset_(clockOffset),
     pageScanMode_(pageScanMode),
     epochs_()
{
}

EbNDeviceBT2::Epoch::Epoch(uint32_t advertNum, uint64_t advertTime, size_t keySize)
   : lastAdvertNum(advertNum),
     lastAdvertTime(advertTime),
     dhRemotePublic(keySize / 8, 0)
{
}
