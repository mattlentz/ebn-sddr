#ifndef TIMING_H
#define TIMING_H

#include <cstdint>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define TIME_MIN_TO_SEC(t) ((t) * 60)
#define TIME_MIN_TO_MS(t) ((t) * 60000)
#define TIME_MIN_TO_US(t) ((t) * 60000000)
#define TIME_SEC_TO_MIN(t) ((t) / 60)
#define TIME_SEC_TO_MS(t) ((t) * 1000)
#define TIME_SEC_TO_US(t) ((t) * 1000000)
#define TIME_MS_TO_MIN(t) ((t) / 60000)
#define TIME_MS_TO_SEC(t) ((t) / 1000)
#define TIME_MS_TO_US(t) ((t) * 1000)

inline uint64_t getTimeSec()
{
  struct timespec curTime;
  clock_gettime(CLOCK_REALTIME, &curTime);
  return curTime.tv_sec;
}

inline uint64_t getTimeMS()
{
  struct timespec curTime;
  clock_gettime(CLOCK_REALTIME, &curTime);
  return ((uint64_t)curTime.tv_sec * 1000) + (curTime.tv_nsec / 1000000);
}

inline uint64_t getTimeUS()
{
  struct timespec curTime;
  clock_gettime(CLOCK_REALTIME, &curTime);
  return ((uint64_t)curTime.tv_sec * 1000000) + (curTime.tv_nsec / 1000);
}

inline uint64_t getMonoSec()
{
  struct timespec curTime;
  clock_gettime(CLOCK_MONOTONIC, &curTime);
  return curTime.tv_sec;
}

inline uint64_t getMonoMS()
{
  struct timespec curTime;
  clock_gettime(CLOCK_MONOTONIC, &curTime);
  return ((uint64_t)curTime.tv_sec * 1000) + (curTime.tv_nsec / 1000000);
}

inline uint64_t getMonoUS()
{
  struct timespec curTime;
  clock_gettime(CLOCK_MONOTONIC, &curTime);
  return ((uint64_t)curTime.tv_sec * 1000000) + (curTime.tv_nsec / 1000);
}

inline void sleepSec(uint64_t time)
{
  usleep(time * 1000000);
}

inline void sleepMS(uint64_t time)
{
  usleep(time * 1000);
}

inline void sleepUS(uint64_t time)
{
  usleep(time);
}

#endif  // TIMING_H
