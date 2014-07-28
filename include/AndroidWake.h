#ifndef ANDROIDWAKE_H
#define ANDROIDWAKE_H

#include <stdint.h>
#include <string>
#include <sys/time.h>
#include <unordered_set>

class AndroidWake
{
private:
  static std::unordered_set<std::string> grabbed_;

private:
  int sock_;

public:
  AndroidWake();
  ~AndroidWake();

  bool sleep(uint64_t msToSleep);

  static void grab(std::string name);
  static void release(std::string name);

private:
  uint64_t curTimeDiff(struct timeval begin);
};

#endif // ANDROIDWAKE_H
