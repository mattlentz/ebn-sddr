#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>

#include "EbNRadio.h"
#include "EbNHystPolicy.h"

struct Config
{
  struct Radio
  {
    size_t keySize;
    EbNRadio::Version version;
    EbNRadio::ConfirmScheme confirm;
    EbNRadio::MemoryScheme memory;
  } radio;

  struct HystPolicy
  {
    EbNHystPolicy::Scheme scheme;
    uint64_t minStartTime; // ms
    uint64_t maxStartTime; // ms
    size_t startSeen;
    uint64_t endTime;      // ms
    int8_t rssiThreshold;
  } hyst;

  struct Reporting
  {
    uint64_t rssiInterval; // ms
  } reporting;

  void dump() const;
};

constexpr Config configDefaults =
{
  {192, EbNRadio::Version::Bluetooth2, {EbNRadio::ConfirmScheme::Passive, 0.05}, EbNRadio::MemoryScheme::Standard},
  {EbNHystPolicy::Scheme::Standard, TIME_MIN_TO_MS(2), TIME_MIN_TO_MS(5), 2, TIME_MIN_TO_MS(10), -85},
  {TIME_MIN_TO_MS(1)}
};

#endif // CONFIG_H
