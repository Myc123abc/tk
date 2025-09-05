//
// log system
//
// only have control log function.
//
// TODO:
// expand other functions like file log.
//

#pragma once

#include <print>
#include <cassert>

namespace tk{ namespace log {

enum class Level
{
  error,
  info,
  warn,
};

#define Red(x)    "\e[31m" x "\e[0m"
#define Green(x)  "\e[32m" x "\e[0m"
#define Yellow(x) "\e[33m" x "\e[0m"

template <typename T>
void log(Level level, T const& msg)
{
  if (level == Level::error)
  std::println("[{}] {}", Red("error"), msg);
  else if (level == Level::info)
  std::println("[{}] {}", Green("info"), msg);
  else if (level == Level::warn)
  std::println("[{}] {}", Yellow("warn"), msg);
  else
  assert(false);
}

template <typename... Args>
void log(Level level, std::format_string<Args...> fmt, Args&&... args)
{
  if (level == Level::error)
  std::println("[{}] {}", Red("error"), fmt, std::forward<Args>(args)...);
  else if (level == Level::info)
  std::println("[{}] {}", Green("info"), fmt, std::forward<Args>(args)...);
  else if (level == Level::warn)
  std::println("[{}] {}", Yellow("warn"), fmt, std::forward<Args>(args)...);
  else
  assert(false);
}

template <typename T>
void error(T const& msg)
{
  log(Level::error, msg);
}

template <typename... Args>
void error(std::format_string<Args...> fmt, Args&&... args)
{
  log(Level::error, std::format(fmt, std::forward<Args>(args)...));
}

template <typename T>
void info(T const& msg)
{
  log(Level::info, msg);
}

template <typename... Args>
void info(std::format_string<Args...> fmt, Args&&... args)
{
  log(Level::info, std::format(fmt, std::forward<Args>(args)...));
}

template <typename T>
void warn(T const& msg)
{
  log(Level::warn, msg);
}

template <typename... Args>
void warn(std::format_string<Args...> fmt, Args&&... args)
{
  log(Level::warn, std::format(fmt, std::forward<Args>(args)...));
}

}}