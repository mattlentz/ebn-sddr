#ifndef EBNDEVICE_H
#define EBNDEVICE_H

#include <algorithm>
#include <cstdint>
#include <list>
#include <string>
#include <vector>

#include "Address.h"
#include "BloomFilter.h"
#include "EbNEvents.h"
#include "LinkValue.h"
#include "SharedArray.h"

class EbNDevice
{
protected:
  DeviceID id_;
  Address address_;
  LinkValueList matching_;
  bool updatedMatching_;
  float matchingPFalse_;
  SharedSecretList sharedSecrets_;
  SharedSecretList secretsToReport_;
  std::list<RSSIEvent> rssiToReport_;
  uint64_t lastReportTime_;
  bool confirmed_;
  bool shakenHands_;
  bool reported_;

public:
  EbNDevice(DeviceID id, const Address &address, const LinkValueList &listenSet);
  ~EbNDevice();

  DeviceID getID() const;

  const Address& getAddress() const;
  virtual void setAddress(const Address &address);

  const LinkValueList& getMatching() const;
  void updateMatching(const BloomFilter *bloom, const uint8_t *prefix, uint32_t prefixSize);
  void updateMatching(const BloomFilter *bloom, const uint8_t *prefix, uint32_t prefixSize, float pFalseDelta);
  float getMatchingPFalse() const;

  const SharedSecretList& getSharedSecrets() const;
  void addSharedSecret(const SharedSecret &secret);
  void confirmPassive(const BloomFilter *bloom, const uint8_t *prefix, uint32_t prefixSize, float threshold);
  void confirmPassive(const BloomFilter *bloom, const uint8_t *prefix, uint32_t prefixSize, float threshold, float pFalseDelta);
  bool isConfirmed() const;

  void setShakenHands(bool value);
  bool hasShakenHands() const;

  void addRSSIMeasurement(uint64_t time, uint8_t rssi);

  bool getEncounterInfo(EncounterEvent &dest, bool expired = false);
  bool getEncounterInfo(EncounterEvent &dest, uint64_t rssiReportingInterval, bool expired = false);
};

inline DeviceID EbNDevice::getID() const
{
  return id_;
}

inline const Address& EbNDevice::getAddress() const
{
  return address_;
}

inline void EbNDevice::setAddress(const Address &address)
{
  address_ = address;
}

inline const LinkValueList& EbNDevice::getMatching() const
{
  return matching_;
}

inline float EbNDevice::getMatchingPFalse() const
{
  return matchingPFalse_;
}

inline const SharedSecretList& EbNDevice::getSharedSecrets() const
{
  return sharedSecrets_;
}

inline bool EbNDevice::isConfirmed() const
{
  return confirmed_;
}

inline void EbNDevice::setShakenHands(bool value)
{
  shakenHands_ = value;
}

inline bool EbNDevice::hasShakenHands() const
{
  return shakenHands_;
}

inline void EbNDevice::addRSSIMeasurement(uint64_t time, uint8_t rssi)
{
  rssiToReport_.push_back(RSSIEvent(time, rssi));
}

#endif // EBNDEVICE_H
