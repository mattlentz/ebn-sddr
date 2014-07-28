#ifndef EBNDEVICEBT4AR_H
#define EBNDEVICEBT4AR_H

#include <cstdint>
#include <list>

#include "EbNDevice.h"
#include "ECDH.h"

class EbNDeviceBT4AR : public EbNDevice
{
private:
  ECDH dhExchange_;

public:
  EbNDeviceBT4AR(DeviceID id, const Address &address, const LinkValueList &listenSet);

  void setAddress(const Address &address);

private:
  bool updateMatching();
}; 

#endif // EBNDEVICEBT4AR_H
