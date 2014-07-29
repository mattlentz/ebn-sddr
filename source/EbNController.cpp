#include "EbNController.h"

#include <stdexcept>

#include "EbNRadioBT2.h"
#include "EbNRadioBT2NR.h"
#include "EbNRadioBT2PSI.h"
#include "EbNRadioBT4.h"
#include "EbNRadioBT4AR.h"
#include "Logger.h"
#include "Timing.h"

using namespace std;

EbNController::EbNController(shared_ptr<EbNRadio> radio, EbNHystPolicy hystPolicy, uint64_t rssiReportInterval)
   : radio_(radio),
     hystPolicy_(hystPolicy),
     rssiReportInterval_(rssiReportInterval),
     encounterCallback_(encounterDefault),
     sleepCallback_(sleepMS),
     isRunning_(false)
{
}

void EbNController::setEncounterCallback(const function<void(const EncounterEvent&)> &callback)
{
  encounterCallback_ = callback;
}

void EbNController::setSleepCallback(const function<void(uint64_t)> &callback)
{
  sleepCallback_ = callback;
}

void EbNController::run()
{
  isRunning_ = true;

  radio_->initialize();

  while(isRunning_)
  {
    EbNRadio::ActionInfo actionInfo = radio_->getNextAction();

    LOG_D("EbNController", "Next action is %s after %" PRIu64 " ms", (actionInfo.action == EbNRadio::Action::ChangeEpoch) ? "ChangeEpoch" : "Discover", actionInfo.timeUntil);

    if(actionInfo.timeUntil > 0)
    {
      LOG_D("EbNController", "Sleeping for %" PRIi64 " ms", actionInfo.timeUntil);
      sleepCallback_(actionInfo.timeUntil);
    }
    else
    {
      LOG_D("EbNController", "Executing the action immediately");
    }

    switch(actionInfo.action)
    {
    case EbNRadio::Action::ChangeEpoch:
      radio_->changeEpoch();
      break;

    case EbNRadio::Action::Discover:
      {
        list<DiscoverEvent> discovered = radio_->discover();
        list<pair<DeviceID, uint64_t> > newlyDiscovered;
        set<DeviceID> toHandshake = hystPolicy_.discovered(discovered, newlyDiscovered);
        hystPolicy_.encountered(radio_->handshake(toHandshake));

        list<EncounterEvent> encounters;

        for(auto ndIt = newlyDiscovered.begin(); ndIt != newlyDiscovered.end(); ndIt++)
        {
          EncounterEvent event(EncounterEvent::UnconfirmedStarted, ndIt->second, ndIt->first);
          encounters.push_back(event);
        }

        for(auto discIt = discovered.begin(); discIt != discovered.end(); discIt++)
        {
          EncounterEvent event(getTimeMS());
          if(radio_->getDeviceEvent(event, discIt->id, rssiReportInterval_))
          {
            encounters.push_back(event);
          }
        }

        list<pair<DeviceID, uint64_t> > expired = hystPolicy_.checkExpired();
        for(auto expIt = expired.begin(); expIt != expired.end(); expIt++)
        {
          EncounterEvent expireEvent = radio_->doneWithDevice(expIt->first);
          expireEvent.time = expIt->second;
          encounters.push_back(expireEvent);
        }

        for(auto encIt = encounters.begin(); encIt != encounters.end(); encIt++)
        {
          encounterCallback_(*encIt);
        }
      }
      break;
    }
  }

  LOG_D("EbNController", "Stopped");
}

void EbNController::encounterDefault(const EncounterEvent &event)
{
  LOG_P("EbNController", "Encounter event took place");
  switch(event.type)
  {
  case EncounterEvent::UnconfirmedStarted:
    LOG_P("EbNController", "Type = UnconfirmedStarted");
    break;
  case EncounterEvent::Started:
    LOG_P("EbNController", "Type = Started");
    break;
  case EncounterEvent::Updated:
    LOG_P("EbNController", "Type = Updated");
    break;
  case EncounterEvent::Ended:
    LOG_P("EbNController", "Type = Ended");
    break;
  }
  LOG_P("EbNController", "ID = %d", event.id);
  LOG_P("EbNController", "Time = %" PRIu64, event.time);
  LOG_P("EbNController", "# Shared Secrets = %d (Updated = %s)", event.sharedSecrets.size(), event.sharedSecretsUpdated ? "true" : "false");
  LOG_P("EbNController", "# Matching Set Entries = %d (Updated = %s)", event.matching.size(), event.matchingSetUpdated ? "true" : "false");
  LOG_P("EbNController", "# RSSI Entries = %d", event.rssiEvents.size());
}

