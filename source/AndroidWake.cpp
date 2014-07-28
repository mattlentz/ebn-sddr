#include <cstdlib>
#include <cstdio>
#include <endian.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "AndroidWake.h"
#include "Logger.h"
#include "Timing.h"

using namespace std;

std::unordered_set<std::string> AndroidWake::grabbed_;

AndroidWake::AndroidWake()
  : sock_(0)
{
  sock_ = socket(AF_UNIX, SOCK_STREAM, 0);
  if(sock_ < 0)
  {
    LOG_D("AndroidWake", "Could not open socket");
    exit(1);
  }

  struct sockaddr_un localAddr;
  localAddr.sun_family = AF_LOCAL;
  localAddr.sun_path[0] = '\0';
  strcpy(&localAddr.sun_path[1], "AWake");

  while(connect(sock_, (struct sockaddr *)&localAddr, sizeof(localAddr.sun_family) + 1 + strlen(&localAddr.sun_path[1])) < 0)
  {
    LOG_D("AndroidWake", "Could not connect to 'AWake' local socket, trying again after 10 seconds");
    sleepSec(10);
  }

  LOG_D("AndroidWake", "Initialized");
}

AndroidWake::~AndroidWake()
{
  if(sock_ >= 0)
  {
    close(sock_);
  }

  // TODO: Can we release all of them at once?
  for(auto it = grabbed_.cbegin(); it != grabbed_.cend(); it++)
  {
    release(*it);
  }
}

void AndroidWake::grab(string name)
{
  char command[128];
  sprintf(command, "echo \"%s\" > /sys/power/wake_lock", name.c_str());
  system(command);

  grabbed_.insert(name);
}

void AndroidWake::release(string name)
{
  char command[128];
  sprintf(command, "echo \"%s\" > /sys/power/wake_unlock", name.c_str());
  system(command);

  grabbed_.erase(name);
}

bool AndroidWake::sleep(uint64_t msToSleep)
{
  struct timeval startTime;
  gettimeofday(&startTime, NULL);

  uint64_t diff;
  while((diff = curTimeDiff(startTime)) < msToSleep)
  {
    uint32_t leftToSleep = msToSleep - diff;
    LOG_D("AndroidWake", "Sleeping for %d ms", leftToSleep);

    uint32_t leftToSleepWrite = htole32(leftToSleep);
    if(write(sock_, (uint8_t *)&leftToSleepWrite, 4) < 4)
    {
      LOG_D("AndroidWake", "Could not write to 'AWake' local socket");
      return false;
    }

    uint8_t inBuffer[1];
    if(read(sock_, inBuffer, 1) < 1)
    {
      LOG_D("AndroidWake", "Could not read from 'AWake' local socket");
      return false;
    }

    LOG_D("AndroidWake", "Woke up");
  }

  return true;
}

uint64_t AndroidWake::curTimeDiff(struct timeval begin)
{
  struct timeval curTime;
  gettimeofday(&curTime, NULL);

  return ((curTime.tv_sec - begin.tv_sec) * 1000) + ((curTime.tv_usec - begin.tv_usec) / 1000);
}
