#ifndef LOGGER_H
#define LOGGER_H

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <sstream>
#include <sys/time.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "Timing.h"

class Logger
{
public:
  typedef enum { MESSAGE_ERROR, MESSAGE_WARNING, MESSAGE_DEBUG, MESSAGE_PARSE } MessageType;
  static const char *messageTypeStrings[];

  static const int androidMessageTypes[];

private:
  typedef std::unique_ptr<FILE, decltype(&fclose)> FilePtr;

private:
  bool screenEnabled_;
  bool logcatEnabled_;
  bool fileEnabled_;
  FilePtr fout_;
  uint64_t initTime_;

public:
  Logger();
  Logger(const char *filename);

  void setScreenEnabled(bool value);
  void setLogcatEnabled(bool value);
  bool setFileEnabled(bool value, const char *filename = NULL);

  bool isScreenEnabled() const;
  bool isLogcatEnabled() const;
  bool isFileEnabled() const;

  void write(MessageType type, const char *tag, const char *format, ...);
};

inline void Logger::setLogcatEnabled(bool value)
{
  logcatEnabled_ = value;
}

inline bool Logger::isScreenEnabled() const
{
  return screenEnabled_;
}

inline bool Logger::isLogcatEnabled() const
{
  return logcatEnabled_;
}

inline bool Logger::isFileEnabled() const
{
  return fileEnabled_;
}

// Global logger instance. Note that for simulation purposes, the simulator will
// change this instance whenever it calls a function for a specific device in
// order to separate the log files.
extern Logger *logger;

#ifdef LOG_W_ENABLED
#define IS_LOG_W_ENABLED 1
#else
#define IS_LOG_W_ENABLED 0
#endif

#ifdef LOG_D_ENABLED
#define IS_LOG_D_ENABLED 1
#else
#define IS_LOG_D_ENABLED 0
#endif

#ifdef LOG_P_ENABLED
#define IS_LOG_P_ENABLED 1
#else
#define IS_LOG_P_ENABLED 0
#endif

// Logging for error messages (Always enabled)
#define LOG_E(tag, format, args...) logger->write(Logger::MESSAGE_ERROR, tag, format , ##args)

// Logging for warning messages
#define LOG_W(tag, format, args...) if(IS_LOG_W_ENABLED) { logger->write(Logger::MESSAGE_WARNING, tag, format , ##args); }

// Logging for debugging messages
#define LOG_D(tag, format, args...) if(IS_LOG_D_ENABLED) { logger->write(Logger::MESSAGE_DEBUG, tag, format , ##args); }

// Logging for any messages required in parsing scripts (e.g. simulations)
#define LOG_P(tag, format, args...) if(IS_LOG_P_ENABLED) { logger->write(Logger::MESSAGE_PARSE, tag, format , ##args); }

#endif  // LOGGER_H

