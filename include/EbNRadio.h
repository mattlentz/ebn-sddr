#ifndef EBNRADIO_H
#define EBNRADIO_H

#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <unordered_map>

#include "EbNDevice.h"
#include "Timing.h"

class EbNRadio
{
protected:
  static const uint32_t BF_N = 256;
  static const uint32_t BF_N_PASSIVE = 128;
  static const uint32_t EPOCH_INTERVAL = TIME_MIN_TO_MS(15);

public:
  struct Version_
  {
    enum Type
    {
      Bluetooth2,
      Bluetooth2NR,
      Bluetooth2PSI,
      Bluetooth4,
      Bluetooth4AR,
      END
    };
  };
  typedef Version_::Type Version;
  static const char *versionStrings[];
  static const char *versionFullStrings[];
  static Version stringToVersion(const char* name);

  struct ConfirmScheme
  {
    enum Type
    {
      None = 0,
      Passive = 1,
      Active = 2,
      Hybrid = 3,
      END
    } type;
    float threshold;
  };
  static const char *confirmSchemeStrings[];
  static ConfirmScheme::Type stringToConfirmScheme(const char* name);

  struct MemoryScheme_
  {
    enum Type
    {
      Standard,
      NoMemory
    };
  };
  typedef MemoryScheme_::Type MemoryScheme;
  static const char *memorySchemeStrings[];

  struct Action_
  {
    enum Type
    {
      ChangeEpoch,
      Discover,
      Handshake
    };
  };
  typedef Action_::Type Action;

  struct ActionInfo
  {
    Action action;
    int64_t timeUntil;

    ActionInfo(Action action, int64_t timeUntil)
       : action(action),
         timeUntil(timeUntil)
    {
    }
  };

protected:
  typedef std::list<EbNDevice *> RecentDeviceList;
  typedef std::unordered_map<DeviceID, RecentDeviceList::iterator> IDToRecentDeviceMap;
  typedef std::priority_queue<SharedSecret, std::vector<SharedSecret>, SharedSecret::Compare> SharedSecretQueue;

protected:
  DeviceID nextDeviceID_;
  size_t keySize_;
  ConfirmScheme confirmScheme_;
  MemoryScheme memoryScheme_;
  LinkValueList advertisedSet_;
  LinkValueList listenSet_;
  std::mutex setMutex_;
  RecentDeviceList recentDevices_;
  IDToRecentDeviceMap idToRecentDevices_;
  uint64_t nextDiscover_;
  uint64_t nextChangeEpoch_;

public:
  EbNRadio(size_t keySize, ConfirmScheme confirmScheme, MemoryScheme memoryScheme);
  virtual ~EbNRadio();

  virtual void setAdvertisedSet(const LinkValueList &advertisedSet);
  virtual void setListenSet(const LinkValueList &listenSet);

  ActionInfo getNextAction();
  ConfirmScheme::Type getHandshakeScheme();

  virtual void initialize() = 0;
  virtual std::list<DiscoverEvent> discover() = 0;
  virtual void changeEpoch() = 0;
  virtual std::set<DeviceID> handshake(const std::set<DeviceID> &ids) = 0;
  virtual EncounterEvent doneWithDevice(DeviceID id) = 0;

  bool getDeviceEvent(EncounterEvent &event, DeviceID id, uint64_t rssiReportInterval);

protected:
  DeviceID generateDeviceID();

  void fillBloomFilter(BloomFilter *bloom, const uint8_t *prefix, uint32_t prefixSize, bool includePassive = true);

  void addRecentDevice(EbNDevice *device);
  void removeRecentDevice(DeviceID id);
};

inline DeviceID EbNRadio::generateDeviceID()
{
  return nextDeviceID_++;
}

#endif  // EBNRADIO_H

