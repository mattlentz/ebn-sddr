#ifndef EBNHYSTPOLICY_H
#define EBNHYSTPOLICY_H

#include <list>
#include <set>
#include <unordered_map>

#include "EbNDevice.h"
#include "EbNEvents.h"

class EbNHystPolicy
{
public:
  struct Scheme_
  {
    enum Type
    {
      Standard = 0,
      Immediate = 1,
      ImmediateNoMem = 3,
      END
    };
  };
  typedef Scheme_::Type Scheme;
  static const char *schemeStrings[];

private:
  struct HystState_
  {
    enum Type
    {
      Discovered,
      Encountered
    };
  };
  typedef HystState_::Type HystState;

  struct DeviceInfo
  {
    uint64_t firstTime;
    uint64_t lastTime;
    size_t seen;
    HystState state;

    DeviceInfo(uint64_t time)
       : firstTime(time),
         lastTime(time),
         seen(0),
         state(HystState::Discovered)
    {
    }

    DeviceInfo(uint64_t time, HystState state)
       : firstTime(time),
         lastTime(time),
         seen(0),
         state(state)
    {
    }
  };

  typedef std::unordered_map<DeviceID, DeviceInfo> DeviceInfoMap;

private:
  Scheme scheme_;
  uint64_t minStartTime_;
  uint64_t maxStartTime_;
  size_t startSeen_;
  uint64_t endTime_;
  int8_t rssiThreshold_;

  DeviceInfoMap deviceInfo_;

public:
  EbNHystPolicy(Scheme scheme, uint64_t minStartTime, uint64_t maxStartTime, size_t startSeen, uint64_t endTime, int8_t rssiThreshold);

  std::set<DeviceID> discovered(const std::list<DiscoverEvent>& events, std::list<std::pair<DeviceID, uint64_t>>& newlyDiscovered);
  void encountered(const std::set<DeviceID> &devices);
  std::list<std::pair<DeviceID, uint64_t> > checkExpired();

  uint64_t getStartTime(DeviceID id);
  uint64_t getLastTime(DeviceID id);
};

#endif // EBNHYSTPOLICY_H
