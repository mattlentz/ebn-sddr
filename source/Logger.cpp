#include "Logger.h"

#include <android/log.h>
#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <stdexcept>
#include <string>

using namespace std;

Logger *logger = new Logger();

const char *Logger::messageTypeStrings[] = { "E", "W", "D", "P" };

const int Logger::androidMessageTypes[] = { ANDROID_LOG_ERROR, ANDROID_LOG_WARN, ANDROID_LOG_DEBUG, ANDROID_LOG_VERBOSE };

void setupSymlink(const string& filename)
{
#ifdef ANDROID
    const string symlinkName = "/data/local/log_latest.txt";
    system((string("rm ") + symlinkName).c_str());
    system((string("ln -s ") + string(filename) + string(" ") + symlinkName).c_str());
#endif
}

Logger::Logger()
   : screenEnabled_(true),
     logcatEnabled_(false),
     fileEnabled_(false),
     fout_(NULL, &fclose),
     initTime_(getTimeMS())
{
}

Logger::Logger(const char *filename)
   : screenEnabled_(true),
     logcatEnabled_(false),
     fileEnabled_(true),
     fout_(fopen(filename, "w"), &fclose),
     initTime_(getTimeMS())
{
}

void Logger::setScreenEnabled(bool value)
{
  if(screenEnabled_ && !value)
  {
    fflush(stdout);
  }

  screenEnabled_ = value;
}

bool Logger::setFileEnabled(bool value, const char *filename)
{
  if(fileEnabled_ && !value)
  {
    fclose(fout_.get());
    fout_.reset(NULL);
  }
  else if(!fileEnabled_ && value)
  {
    fout_.reset(fopen(filename, "w"));
  }

  fileEnabled_ = (fout_.get() != NULL);

  if (fileEnabled_) {
    setupSymlink(string(filename));
  }

  return fileEnabled_;
}

void Logger::write(MessageType type, const char *tag, const char *format, ...)
{
  uint64_t timeDiff = getTimeMS() - initTime_;

  va_list args;
  va_start(args, format);

  if(screenEnabled_)
  {
    printf("[%" PRIu64 "] %s(%s) - ", timeDiff, tag, messageTypeStrings[type]);
    vprintf(format, args);
    printf("\n");
    fflush(stdout);
  }

  if(logcatEnabled_)
  {
    __android_log_vprint(androidMessageTypes[type], tag, format, args);
  }

  if(fileEnabled_)
  {
    fprintf(fout_.get(), "[%" PRIu64 "] %s(%s) - ", timeDiff, tag, messageTypeStrings[type]);
    vfprintf(fout_.get(), format, args);
    fprintf(fout_.get(), "\n");
    fflush(fout_.get());
  }

  va_end(args);
}
