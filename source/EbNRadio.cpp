#include "EbNRadio.h"

#include "Logger.h"

using namespace std;

const char *EbNRadio::versionStrings[] = { "BT2", "BT2NR", "BT2PSI", "BT4", "BT4AR" };
const char *EbNRadio::versionFullStrings[] = { "Bluetooth 2.1", "Bluetooth 2.1 Name Request",
                                               "Bluetooth 2.1 Private Set Intersection (PSI)",
                                               "Bluetooth 4.0", "Bluetooth 4.0 Address Resolution" };
const char *EbNRadio::confirmSchemeStrings[] = { "None", "Passive", "Active", "Hybrid" };
const char *EbNRadio::memorySchemeStrings[] = { "Standard", "No Memory" };

EbNRadio::Version EbNRadio::stringToVersion(const char* name)
{
  Version version = Version::END;

  for(int v = 0; v < EbNRadio::Version::END; v++)
  {
    if(strcmp(name, EbNRadio::versionStrings[v]) == 0)
    {
      version = (Version)v;
    }
  }

  return version;
}

EbNRadio::ConfirmScheme::Type EbNRadio::stringToConfirmScheme(const char* name)
{
  ConfirmScheme::Type type = ConfirmScheme::END;

  for(int t = 0; t < EbNRadio::ConfirmScheme::END; t++)
  {
    if(strcmp(name, EbNRadio::confirmSchemeStrings[t]) == 0)
    {
      type = (ConfirmScheme::Type)t;
    }
  }

  return type;
}

EbNRadio::EbNRadio(size_t keySize, ConfirmScheme confirmScheme, MemoryScheme memoryScheme)
   : nextDeviceID_(0),
     keySize_(keySize),
     confirmScheme_(confirmScheme),
     memoryScheme_(memoryScheme),
     advertisedSet_(),
     listenSet_(),
     setMutex_(),
     recentDevices_(),
     idToRecentDevices_(),
     nextDiscover_(getTimeMS() + 10000),
     nextChangeEpoch_(getTimeMS() + EPOCH_INTERVAL)
{
}

EbNRadio::~EbNRadio()
{
}

void EbNRadio::setAdvertisedSet(const LinkValueList &advertisedSet)
{
  lock_guard<mutex> setLock(setMutex_);
  advertisedSet_ = advertisedSet;
}

void EbNRadio::setListenSet(const LinkValueList &listenSet)
{
  lock_guard<mutex> setLock(setMutex_);
  listenSet_ = listenSet;
}

void EbNRadio::fillBloomFilter(BloomFilter *bloom, const uint8_t *prefix, uint32_t prefixSize, bool includePassive)
{
  int numRandom = 0;

  // Inserting link values from the advertised set
  int maxAdvert = BF_N;
  if((confirmScheme_.type & ConfirmScheme::Passive) != 0)
  {
    maxAdvert = BF_N - BF_N_PASSIVE;
  }

  unique_lock<mutex> setLock(setMutex_);

  int numAdvert = 0;
  for(LinkValueList::iterator it = advertisedSet_.begin(); (it != advertisedSet_.end()) && (numAdvert < maxAdvert);  it++, numAdvert++)
  {
    bloom->add(prefix, prefixSize, it->get(), it->size());
  }

  setLock.unlock();

  numRandom += (maxAdvert - numAdvert);

  // Inserting link values for passive confirmation, using the most confident
  // shared secrets from all recently discovered devices. This corresponds to
  // the "Highest Confidence (HC)" scheme mentioned in the paper.
  int numPassive = 0;
  if(includePassive && ((confirmScheme_.type & ConfirmScheme::Passive) != 0))
  {
    SharedSecretQueue secrets;

    for(auto devIt = recentDevices_.begin(); devIt != recentDevices_.end(); devIt++)
    {
      const SharedSecretList &deviceSecrets = (*devIt)->getSharedSecrets();
      for(auto secIt = deviceSecrets.begin(); secIt != deviceSecrets.end(); secIt++)
      {
        // Only include secrets which were not actively confirmed
        if(secIt->confirmedBy != SharedSecret::ConfirmScheme::Active)
        {
          secrets.push(*secIt);
        }
      }
    }

    while(!secrets.empty() && (numPassive < BF_N_PASSIVE))
    {
      const SharedSecret &secret = secrets.top();
      bloom->add(prefix, prefixSize, secret.value.get(), secret.value.size());
      secrets.pop();
      numPassive++;
    }

    numRandom += (BF_N_PASSIVE - numPassive);
  }

  // Inserting random link values to ensure constant Bloom filter load
  bloom->addRandom(numRandom);

  LOG_D("EbNRadio", "Created new Bloom filter (Advertised %d, Passive %d, Random %d)", numAdvert, numPassive, numRandom);
}

void EbNRadio::addRecentDevice(EbNDevice *device)
{
  removeRecentDevice(device->getID());

  recentDevices_.push_front(device);
  idToRecentDevices_.insert(make_pair(device->getID(), recentDevices_.begin()));
}

void EbNRadio::removeRecentDevice(DeviceID id)
{
  IDToRecentDeviceMap::iterator it = idToRecentDevices_.find(id);
  if(it != idToRecentDevices_.end())
  {
    recentDevices_.erase(it->second);
    idToRecentDevices_.erase(it);
  }
}

EbNRadio::ActionInfo EbNRadio::getNextAction()
{
  int64_t timeUntil = 0;
  const uint64_t curTime = getTimeMS();

  if(nextChangeEpoch_ < nextDiscover_)
  {
    if(nextChangeEpoch_ > curTime)
    {
      timeUntil = nextChangeEpoch_ - curTime;
    }
    return ActionInfo(Action::ChangeEpoch, timeUntil);
  }
  else
  {
    if(nextDiscover_ > curTime)
    {
      timeUntil = nextDiscover_ - curTime;
    }
    return ActionInfo(Action::Discover, timeUntil);
  }
}

EbNRadio::ConfirmScheme::Type EbNRadio::getHandshakeScheme()
{
  ConfirmScheme::Type type;

  if(confirmScheme_.type == ConfirmScheme::Hybrid)
  {
    // TODO: Need to figure out what a good threshold is in terms of energy
    // consumption and also performance in dense environments
    if(recentDevices_.size() > 128)
    {
      type = ConfirmScheme::Passive;
    }
    else
    {
      type = ConfirmScheme::Active;
    }
  }
  else
  {
    type = confirmScheme_.type;
  }

  return type;
}

bool EbNRadio::getDeviceEvent(EncounterEvent &event, DeviceID id, uint64_t rssiReportInterval)
{
  IDToRecentDeviceMap::iterator it = idToRecentDevices_.find(id);
  if(it != idToRecentDevices_.end())
  {
    EbNDevice *device = *it->second;
    if(device->getEncounterInfo(event, rssiReportInterval))
    {
      return true;
    }
  }

  return false;
}
