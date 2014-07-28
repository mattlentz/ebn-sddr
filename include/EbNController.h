#ifndef EBNCONTROLLER_H
#define EBNCONTROLLER_H

#include <functional>
#include <memory>

#include "Config.h"
#include "EbNHystPolicy.h"
#include "EbNRadio.h"

class EbNController
{
private:
  struct RunningState_
  {
    enum Type
    {
      Running,
      Stopping,
      Stopped
    };
  };
  typedef RunningState_::Type RunningState;

private:
  std::shared_ptr<EbNRadio> radio_;
  EbNHystPolicy hystPolicy_;
  uint64_t rssiReportInterval_;
  std::function<void(const EncounterEvent&)> encounterCallback_;
  std::function<void(uint64_t)> sleepCallback_;
  volatile bool isRunning_;

public:
  EbNController(std::shared_ptr<EbNRadio> radio, EbNHystPolicy hystPolicy, uint64_t rssiReportInterval);

  void setEncounterCallback(const std::function<void(const EncounterEvent&)> &callback);
  void setSleepCallback(const std::function<void(uint64_t)> &callback);

  void setAdvertisedSet(const LinkValueList &advertisedSet);
  void setListenSet(const LinkValueList &listenSet);

  void run();
  void stop();

private:
  static void encounterDefault(const EncounterEvent &event);
};

inline void EbNController::setAdvertisedSet(const LinkValueList &advertisedSet)
{
  radio_->setAdvertisedSet(advertisedSet);
}

inline void EbNController::setListenSet(const LinkValueList &listenSet)
{
  radio_->setListenSet(listenSet);
}

inline void EbNController::stop()
{
  isRunning_ = false;
}

#endif // EBNCONTROLLER_H
