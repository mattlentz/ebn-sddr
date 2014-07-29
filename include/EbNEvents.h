#ifndef EBNEVENTS_H
#define EBNEVENTS_H

#include <cstdint>
#include <list>
#include <string>

#include "LinkValue.h"

typedef int32_t DeviceID;

extern uint64_t sddrStartTimestamp;

struct DiscoverEvent
{
  uint64_t time;
  DeviceID id;
  int8_t rssi;

  struct Compare
  {
    bool operator ()(const DiscoverEvent &a, const DiscoverEvent &b) const
    {
      return a.id < b.id;
    }
  };

  DiscoverEvent(uint64_t time, DeviceID id, int8_t rssi)
     : time(time),
       id(id),
       rssi(rssi)
  {
  }
};

struct RSSIEvent
{
  uint64_t time;
  int8_t rssi;

  RSSIEvent(uint64_t time, int8_t rssi)
     : time(time),
       rssi(rssi)
  {
  }
};

struct EncounterEvent
{
  enum Type
  {
    Started = 0,
    Updated = 1,
    Ended = 2,
    UnconfirmedStarted = 3
  };

  Type type;
  uint64_t time;
  DeviceID id;
  std::string address;
  std::list<RSSIEvent> rssiEvents;
  std::list<LinkValue> matching;
  std::list<SharedSecret> sharedSecrets;
  bool matchingSetUpdated;
  bool sharedSecretsUpdated;

  EncounterEvent(uint64_t time)
     : time(time),
       matchingSetUpdated(false),
       sharedSecretsUpdated(false)
  {
  }

  EncounterEvent(Type type, uint64_t time, DeviceID id)
     : type(type),
       time(time),
       id(id),
       matchingSetUpdated(false),
       sharedSecretsUpdated(false)
  {
  }

  // Unique handle to be used as database primary key
  uint64_t getPKID() const
  {
      return sddrStartTimestamp + id;
  }
};

#endif // EBNEVENTS_H
