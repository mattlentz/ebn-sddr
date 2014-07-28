#include <cstdint>
#include <cstdio>
#include <cutils/properties.h>
#include <endian.h>
#include <functional>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <iostream>
#include <limits>
#include <optionparser/optionparser.h>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdexcept>
#include <thread>
#include <unistd.h>

#include "AndroidWake.h"
#include "Config.h"
#include "EbNController.h"
#include "EbNHystPolicy.h"
#include "EbNRadioBT2.h"
#include "EbNRadioBT2NR.h"
#include "EbNRadioBT2PSI.h"
#include "EbNRadioBT4.h"
#include "EbNRadioBT4AR.h"
#include "Logger.h"
#include "SipHash.h"
#include "Timing.h"

#include "ebncore.pb.h"

using namespace std;
using namespace google::protobuf::io;

uint64_t sddrStartTimestamp = getTimeMS();

void initialize()
{
  FILE *urand = fopen("/dev/urandom", "r");
  unsigned int seed = 0;
  for(int b = 0; b < sizeof(seed); b++)
  {
    seed <<= 8;
    seed |= (uint8_t)getc(urand);
  }
  srand(seed);

  logger->setLogcatEnabled(false);
  logger->setScreenEnabled(true);

#ifdef ANDROID
  stringstream logPathSS;
  logPathSS << "/data/local/log_" << sddrStartTimestamp << ".txt";
  logger->setFileEnabled(true, logPathSS.str().c_str());
#else
  logger->setFileEnabled(true, "log.txt");
#endif

  // Remount for MAGURO, so that we can modify the Bluetooth address
#ifdef MAGURO
  system("mount -o rw,remount /dev/block/platform/omap/omap_hsmmc.0/by-name/efs /factory");
#endif
}

bool sendAll(int sock, const uint8_t *message, size_t size)
{
  bool success = true;
  size_t pos = 0;
  while(success && (pos < size))
  {
    size_t numWritten = write(sock, message + pos, size - pos);
    if(numWritten < 0)
    {
      success = false;
    }
    else
    {
      pos += numWritten;
    }
  }

  return success;
}

bool recvAll(int sock, uint8_t *dest, size_t size)
{
  bool success = true;
  size_t pos = 0;
  while(success && (pos < size))
  {
    size_t numRead = read(sock, dest + pos, size - pos);
    if(numRead < 0)
    {
      success = false;
    }
    else
    {
      pos += numRead;
    }
  }

  return success;
}

void requestSleep(int clientSock, uint64_t duration)
{
  EbNCore::Event_SleepEvent *sleepEvent = new EbNCore::Event_SleepEvent();
  sleepEvent->set_duration(duration);

  EbNCore::Event event;
  event.set_allocated_sleepevent(sleepEvent);

  vector<uint8_t> message(4 + event.ByteSize());
  ArrayOutputStream messageArrayOutput(message.data(), message.size());
  CodedOutputStream messageOutput(&messageArrayOutput);

  uint32_t messageSize = htole32(event.ByteSize());
  messageOutput.WriteRaw((uint8_t *)&messageSize, 4);

  event.SerializeToCodedStream(&messageOutput);

  sendAll(clientSock, message.data(), message.size());

  sleepMS(duration);
}

void requestAWakeSleep(shared_ptr<AndroidWake> awake, uint64_t duration)
{
  awake->sleep(duration);
}

void notifyEncounter(int clientSock, const EncounterEvent &event)
{
  LOG_P("Main", "Encounter event took place");
  switch(event.type)
  {
  case EncounterEvent::UnconfirmedStarted:
    LOG_P("Main", "Type = UnconfirmedStarted");
    break;
  case EncounterEvent::Started:
    LOG_P("Main", "Type = Started");
    break;
  case EncounterEvent::Updated:
    LOG_P("Main", "Type = Updated");
    break;
  case EncounterEvent::Ended:
    LOG_P("Main", "Type = Ended");
    break;
  }
  LOG_P("Main", "ID = %d", event.id);
  LOG_P("Main", "Time = %" PRIu64, event.time);
  LOG_P("Main", "# Shared Secrets = %d (Updated = %s)", event.sharedSecrets.size(), event.sharedSecretsUpdated ? "true" : "false");
  LOG_P("Main", "# Matching Set Entries = %d (Updated = %s)", event.matching.size(), event.matchingSetUpdated ? "true" : "false");
  LOG_P("Main", "# RSSI Entries = %d", event.rssiEvents.size());

  EbNCore::Event_EncounterEvent *encounterEvent = new EbNCore::Event_EncounterEvent();
  encounterEvent->set_type((EbNCore::Event_EncounterEvent_EventType)event.type);
  encounterEvent->set_time(event.time);
  encounterEvent->set_id(event.id);
  encounterEvent->set_pkid(event.getPKID());
  encounterEvent->set_address(event.address);
  encounterEvent->set_matchingsetupdated(event.matchingSetUpdated);

  for(auto it = event.rssiEvents.begin(); it != event.rssiEvents.end(); it++)
  {
    EbNCore::Event_EncounterEvent_RSSIEvent *rssiEvent = encounterEvent->add_rssievents();
    rssiEvent->set_time(it->time);
    rssiEvent->set_rssi(it->rssi);
  }

  for(auto it = event.matching.begin(); it != event.matching.end(); it++)
  {
    encounterEvent->add_matchingset(it->get(), it->size());
  }

  for(auto it = event.sharedSecrets.begin(); it != event.sharedSecrets.end(); it++)
  {
    encounterEvent->add_sharedsecrets(it->value.get(), it->value.size());
  }

  EbNCore::Event fullEvent;
  fullEvent.set_allocated_encounterevent(encounterEvent);

  vector<uint8_t> message(4 + fullEvent.ByteSize());
  ArrayOutputStream messageArrayOutput(message.data(), message.size());
  CodedOutputStream messageOutput(&messageArrayOutput);

  uint32_t messageSize = htole32(fullEvent.ByteSize());
  messageOutput.WriteRaw((uint8_t *)&messageSize, 4);

  fullEvent.SerializeToCodedStream(&messageOutput);

  sendAll(clientSock, message.data(), message.size());
}

struct Arg: public option::Arg
{
  static option::ArgStatus NonEmpty(const option::Option &opt, bool msg)
  {
    if((opt.arg != NULL) && (opt.arg[0] != 0))
    {
      return option::ARG_OK;
    }

    if(msg)
    {
      LOG_E("Options", "Option %s requires an argument.", opt.name);
    }
    return option::ARG_ILLEGAL;
  }

  static option::ArgStatus Radio(const option::Option &opt, bool msg)
  {
    if((opt.arg != NULL) && (opt.arg[0] != 0))
    {
      EbNRadio::Version version = EbNRadio::stringToVersion(opt.arg);
      if(version != EbNRadio::Version::END)
      {
        return option::ARG_OK;
      }
    }

    if(msg)
    {
      LOG_E("Options", "Option %s is invalid, must use one of:", opt.name);
      for(int v = 0; v < EbNRadio::Version::END; v++)
      {
        LOG_E("Options", "  %s", EbNRadio::versionStrings[v]);
      }
    }
    return option::ARG_ILLEGAL;
  }

  static option::ArgStatus Confirm(const option::Option &opt, bool msg)
  {
    if((opt.arg != NULL) && (opt.arg[0] != 0))
    {
      EbNRadio::ConfirmScheme::Type type = EbNRadio::stringToConfirmScheme(opt.arg);
      if(type != EbNRadio::ConfirmScheme::END)
      {
        return option::ARG_OK;
      }

      if(msg)
      {
        LOG_E("Options", "Option %s is invalid, must use one of:", opt.name);
        for(int c = 0; c < EbNRadio::ConfirmScheme::END; c++)
        {
          LOG_E("Options", "  %s", EbNRadio::confirmSchemeStrings[c]);
        }
      }

      return option::ARG_ILLEGAL;
    }

    return option::ARG_IGNORE;
  }

  static option::ArgStatus Numeric(const option::Option &opt, bool msg)
  {
    char *end = NULL;
    if(opt.arg != 0)
    {
      strtol(opt.arg, &end, 10);
    }
    if((end != opt.arg) && (*end == 0))
    {
      return option::ARG_OK;
    }

    if(msg)
    {
      LOG_E("Options", "Option %s requires a numeric argument.\n", opt.name);
    }
    return option::ARG_ILLEGAL;
  }

  static option::ArgStatus Unknown(const option::Option& opt, bool msg)
  {
    if(msg)
    {
      LOG_E("Options", "Unknown option %s.\n", opt.name);
    }
    return option::ARG_ILLEGAL;
  }
};

enum optionIndex { UNKNOWN, HELP, RADIO, CONFIRM, BENCH, CHURN, PSICMP };
const option::Descriptor usage[] =
{
  {UNKNOWN, 0,  "",        "", Arg::Unknown,  "USAGE: sddr [options]\n\nOptions:\n"},
  {HELP,    0, "h",    "help", Arg::None,     " --help     (-h)  Print this help information.\n"},
  {RADIO,   0, "r",   "radio", Arg::Radio,    " --radio    (-r)  Radio version to use: BT2, BT2NR, BT2PSI, BT4, BT4AR.\n"
                                              "                  The default is 'BT2'.\n"},
  {CONFIRM, 0, "c", "confirm", Arg::Confirm,  " --confirm  (-c)  Confirmation scheme to use: None, Passive, Active, Hybrid.\n"
                                              "                  The default is 'None' for most radios; however, BT2PSI only\n"
                                              "                  supports 'Active' due to using connection-oriented protocol,\n"
                                              "                  and BT4AR only supports 'None' as it does not generate\n"
                                              "                  shared secrets.\n"},
  {BENCH,   0, "b",   "bench", Arg::Numeric,  " --bench=#  (-b)  Benchmarking mode for generating results, specifying a number\n"
                                              "                  of random entries to create in the advertised/listen sets. In\n"
                                              "                  addition, the client runs without a higher-level application\n"
                                              "                  connected (typically for control and encounter event reports).\n"},
  {CHURN,   0,  "",   "churn", Arg::None,     " --churn    (  )  Act as if each discovered device is new (100% churn in nearby\n"
                                              "                  devices). Additionally, immediately perform a handshake with\n"
                                              "                  each discovered device (no hysteresis policy).\n"},
  {PSICMP,  0,  "",  "psicmp", Arg::Numeric,  " --psicmp=# (  )  Specific benchmarking mode to compare the device recognition\n"
                                              "                  portion of the protocol to standard PSI implementations. Only\n"
                                              "                  available in benchmarking mode, and only for 'BT2' radio. The\n"
                                              "                  value corresponds to how many advertisements to process."},
  {0, 0, 0, 0, 0, 0}
};

shared_ptr<EbNRadio> setupRadio(Config config)
{
  shared_ptr<EbNRadio> radio;
  switch(config.radio.version)
  {
  case EbNRadio::Version::Bluetooth2:
    radio.reset(new EbNRadioBT2(config.radio.keySize, config.radio.confirm, config.radio.memory, 0));
    break;
  case EbNRadio::Version::Bluetooth2NR:
    radio.reset(new EbNRadioBT2NR(config.radio.keySize, config.radio.confirm, config.radio.memory, 0));
    break;
  case EbNRadio::Version::Bluetooth2PSI:
    radio.reset(new EbNRadioBT2PSI(config.radio.keySize, config.radio.confirm, config.radio.memory, 0));
    break;
  case EbNRadio::Version::Bluetooth4:
    radio.reset(new EbNRadioBT4(config.radio.keySize, config.radio.confirm, config.radio.memory, 0));
    break;
  case EbNRadio::Version::Bluetooth4AR:
    radio.reset(new EbNRadioBT4AR(config.radio.keySize, config.radio.confirm, config.radio.memory, 0));
    break;
  }

  return radio;
}

unique_ptr<EbNController> setupController(Config config)
{
  shared_ptr<EbNRadio> radio = setupRadio(config);

  EbNHystPolicy hystPolicy (config.hyst.scheme, config.hyst.minStartTime, config.hyst.maxStartTime,
                            config.hyst.startSeen, config.hyst.endTime, config.hyst.rssiThreshold);

  return unique_ptr<EbNController>(new EbNController(radio, hystPolicy, config.reporting.rssiInterval));
}

int main(int argc, char **argv)
{
  initialize();

  argc -= (argc > 0); argv += (argc > 0);
  option::Stats stats(usage, argc, argv);
  option::Option options[stats.options_max], buffer[stats.buffer_max];
  option::Parser parse(usage, argc, argv, options, buffer);

  try
  {
    if(parse.error())
    {
      return 1;
    }

    if(options[HELP])
    {
      option::printUsage(cout, usage);
      return 0;
    }

    if(options[PSICMP] && !options[BENCH])
    {
      LOG_E("Options", "Option --psicmp requires benchmarking mode (--bench or -b).");
      option::printUsage(cout, usage);
      return 1;
    }

    // Merging specified command line parameters with the default options
    Config config = configDefaults;
    if(options[RADIO])
    {
      config.radio.version = EbNRadio::stringToVersion(options[RADIO].arg);
    }
    if(options[CONFIRM])
    {
      config.radio.confirm.type = EbNRadio::stringToConfirmScheme(options[CONFIRM].arg);
    }
    else
    {
      switch(config.radio.version)
      {
      case EbNRadio::Version::Bluetooth2:
        config.radio.confirm = EbNRadioBT2::getDefaultConfirmScheme();
        break;
      case EbNRadio::Version::Bluetooth2NR:
        config.radio.confirm = EbNRadioBT2NR::getDefaultConfirmScheme();
        break;
      case EbNRadio::Version::Bluetooth2PSI:
        config.radio.confirm = EbNRadioBT2PSI::getDefaultConfirmScheme();
        break;
      case EbNRadio::Version::Bluetooth4:
        config.radio.confirm = EbNRadioBT4::getDefaultConfirmScheme();
        break;
      case EbNRadio::Version::Bluetooth4AR:
        config.radio.confirm = EbNRadioBT4AR::getDefaultConfirmScheme();
        break;
      }
    }

    // Used for benchmarking, acting is if there is 100% churn rate in the set
    // of nearby devices. This involves setting the hysteresis policy to
    // immediately request handshakes (and not remember devices), as well as
    // for the radio to keep no device information around.
    if(options[CHURN])
    {
      config.hyst.scheme = EbNHystPolicy::Scheme::ImmediateNoMem;
      config.radio.memory = EbNRadio::MemoryScheme::NoMemory;
    }

    config.dump();

    // Running in benchmarking mode, without being connected to a
    // higher-level application for control and encounter event reports
    if(options[BENCH])
    {
      char *end;
      int numLinkValues = strtol(options[BENCH].arg, &end, 10);
      size_t linkValueSize = config.radio.keySize / 8;

      LinkValueList randLinkValues;
      for(int n = 0; n < numLinkValues; n++)
      {
        LinkValue linkValue(new uint8_t[linkValueSize], linkValueSize);
        for(int b = 0; b < linkValueSize; b++)
        {
          linkValue[b] = rand() & 0xFF;
        }
        randLinkValues.push_back(linkValue);
      }

      // Running for comparing our Bloom filter-based approach to that of
      // Private Set Intersection (PSI)
      if(options[PSICMP])
      {
        if(config.radio.version != EbNRadio::Version::Bluetooth2)
        {
          throw std::runtime_error("PSI comparison is only supported for the 'Bluetooth2' radio.");
        }

        char *end;
        int numAdverts = strtol(options[PSICMP].arg, &end, 10);

        shared_ptr<EbNRadio> sender = setupRadio(config);
        sender->setAdvertisedSet(randLinkValues);

        shared_ptr<EbNRadio> receiver = setupRadio(config);
        receiver->setListenSet(randLinkValues);

        LOG_P("PSIComparison", "Running for %d link values, over %d advertisements...", numLinkValues, numAdverts);

        // The necessary methods for this PSI comparison benchmark are not a part
        // of the EbNRadio interface, so we must do this work on the lower-level
        // radio instance.
        switch(config.radio.version)
        {
        case EbNRadio::Version::Bluetooth2:
        {
          EbNRadioBT2 *lowSender = dynamic_cast<EbNRadioBT2 *>(sender.get());
          EbNRadioBT2 *lowReceiver = dynamic_cast<EbNRadioBT2 *>(receiver.get());

          for(int r = 0; r < 50; r++)
          {
            EbNDeviceBT2 device(0, Address(), 0, 0, randLinkValues);

            vector<BitMap> adverts(numAdverts);
            for(int a = 0; a < numAdverts; a++)
            {
              adverts[a] = lowSender->generateAdvert(a);
            }

            uint64_t startTime = getTimeUS();
            for(int a = 0; a < numAdverts; a++)
            {
              lowReceiver->processAdvert(&device, getTimeMS(), adverts[a].toByteArray(), false);
              lowReceiver->processEpochs(&device);
            }
            uint64_t stopTime = getTimeUS();
            LOG_P("PSIComparison", "Sample: %" PRIu64 " us", stopTime - startTime);
          }

          break;
        }
        }
      }
      // Standard benchmarking mode
      else
      {
        shared_ptr<AndroidWake> awake(new AndroidWake());

        unique_ptr<EbNController> controller = setupController(config);
        controller->setAdvertisedSet(randLinkValues);
        controller->setListenSet(randLinkValues);
        controller->setSleepCallback(bind(&requestAWakeSleep, awake, placeholders::_1));
        controller->run();
      }
    }
    // Running in standard mode
    else
    {
      GOOGLE_PROTOBUF_VERIFY_VERSION;

      // Setting up the local unix socket server
      int serverSock = socket(AF_UNIX, SOCK_STREAM, 0);
      if(serverSock < 0)
      {
        LOG_E("Main", "Could not open local unix socket");
        exit(1);
      }

      struct sockaddr_un serverAddr;
      serverAddr.sun_family = AF_LOCAL;
      serverAddr.sun_path[0] = '\0';
      strcpy(&serverAddr.sun_path[1], "sddr");
      socklen_t serverAddrLen = sizeof(serverAddr.sun_family) + 1 + strlen(&serverAddr.sun_path[1]);

      int error;
      if((error = bind(serverSock, (struct sockaddr *)&serverAddr, serverAddrLen)) < 0)
      {
        LOG_E("Main", "Implement recovery at %s:%d for error %d", __FILE__, __LINE__, errno);
        exit(1);
      }

      if((error = listen(serverSock, 5)) < 0)
      {
        LOG_E("Main", "Implement recovery at %s:%d for error %d", __FILE__, __LINE__, errno);
        exit(1);
      }

      LOG_D("Main", "Waiting for protobuf client to connect...");

      // Accept loop, handling one client at a time
      int clientSock;
      struct sockaddr_un clientAddr;
      socklen_t clientAddrLen = sizeof(clientAddr);
      while((clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &clientAddrLen)) >= 0)
      {
        LOG_D("Main", "Accepted incoming connection!");
        LOG_D("Main", "Starting up the EbN controller thread...");

        unique_ptr<EbNController> controller = setupController(config);
        controller->setSleepCallback(bind(&requestSleep, clientSock, placeholders::_1));
        controller->setEncounterCallback(bind(&notifyEncounter, clientSock, placeholders::_1));
        thread controllerThread(&EbNController::run, controller.get());

        LOG_D("Main", "Waiting for incoming messages...");

        // Message loop, handling protocol buffer messages
        uint8_t messageSizeBytes[4];
        uint32_t messageSize;
        while(recvAll(clientSock, messageSizeBytes, 4))
        {
          messageSize = letoh32(*((uint32_t *)messageSizeBytes));

          LOG_D("Main", "Receiving a message of size %u", messageSize);

          vector<uint8_t> message(messageSize, 0);
          recvAll(clientSock, message.data(), messageSize);

          EbNCore::Event event;
          CodedInputStream eventInput(message.data(), messageSize);
          event.ParseFromCodedStream(&eventInput);

          // Updating advertised and listen sets
          if(event.has_linkabilityevent())
          {
            LOG_D("Main", "Updating advertised and listen sets");

            LinkValueList advertisedSet;
            LinkValueList listenSet;

            const EbNCore::Event::LinkabilityEvent &subEvent = event.linkabilityevent();
            for(size_t e = 0; e < subEvent.entries_size(); e++)
            {
              const EbNCore::Event_LinkabilityEvent_Entry &entry = subEvent.entries(e);

              const string &linkValueStr = entry.linkvalue();
              LinkValue linkValue(new uint8_t[linkValueStr.length()], linkValueStr.length());
              memcpy(linkValue.get(), linkValueStr.c_str(), linkValueStr.length());

              const EbNCore::Event_LinkabilityEvent_Entry_ModeType &mode = entry.mode();
              switch(mode)
              {
              case EbNCore::Event_LinkabilityEvent_Entry_ModeType_AdvertAndListen:
                advertisedSet.push_back(linkValue);
                LOG_D("Main", "Adding %s to the advertised set", linkValue.toString().c_str());
              case EbNCore::Event_LinkabilityEvent_Entry_ModeType_Listen:
                listenSet.push_back(linkValue);
                LOG_D("Main", "Adding %s to the listen set", linkValue.toString().c_str());
                break;
              default:
                LOG_E("Main", "Received invalid mode (%d) in a linkability event entry", mode);
                throw runtime_error("Invalid linkability entry mode");
                break;
              }
            }

            controller->setAdvertisedSet(advertisedSet);
            controller->setListenSet(listenSet);
          }
        }

        LOG_D("Main", "Connection terminated, stopping the controller");

        controller->stop();
        if(controllerThread.joinable())
        {
          controllerThread.join();
        }

        LOG_D("Main", "Waiting for another protobuf client to connect...");

        close(clientSock);
      }

      LOG_D("Main", "Terminating the server - %s", strerror(errno));

      close(serverSock);

      google::protobuf::ShutdownProtobufLibrary();
    }
  }
  catch(const std::exception &ex)
  {
    LOG_E("Main", "Exception Occurred: %s", ex.what());
  }
  catch(const std::string &ex)
  {
    LOG_E("Main", "Exception Occurred: %s", ex.c_str());
  }
  catch(...)
  {
    LOG_E("Main", "Exception Occurred: UNKNOWN");
  }

  return 0;
}
