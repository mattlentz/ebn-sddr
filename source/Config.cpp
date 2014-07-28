#include "Config.h"

#include "Logger.h"
#include "Timing.h"

void Config::dump() const
{
  LOG_P("Config", "Radio");
  LOG_P("Config", "  Version = %s", EbNRadio::versionFullStrings[radio.version]);
  LOG_P("Config", "  KeySize = %d bits", radio.keySize);
  LOG_P("Config", "  Confirm Scheme = %s", EbNRadio::confirmSchemeStrings[radio.confirm.type]);
  LOG_P("Config", "  Threshold = %g", radio.confirm.threshold);
  LOG_P("Config", "  Memory Scheme = %s", EbNRadio::memorySchemeStrings[radio.memory]);
  LOG_P("Config", "Hysteresis Policy");
  LOG_P("Config", "  Scheme = %s", EbNHystPolicy::schemeStrings[hyst.scheme]);
  LOG_P("Config", "  Start Time (Min) = %" PRIu64 " min", TIME_MS_TO_MIN(hyst.minStartTime));
  LOG_P("Config", "  Start Time (Max) = %" PRIu64 " min", TIME_MS_TO_MIN(hyst.maxStartTime));
  LOG_P("Config", "  Start Seen = %zu", hyst.startSeen);
  LOG_P("Config", "  EndTime = %" PRIu64 " min", TIME_MS_TO_MIN(hyst.endTime));
  LOG_P("Config", "  RSSI Threshold = %d", hyst.rssiThreshold);
  LOG_P("Config", "Reporting");
  LOG_P("Config", "  RSSI Interval = %" PRIu64 " sec", TIME_MS_TO_SEC(reporting.rssiInterval));
}

