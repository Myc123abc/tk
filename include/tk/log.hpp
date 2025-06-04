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

template <typename T>
void log(Level level, T const& msg)
{
  if (level == Level::error)
  std::println("[error] {}", msg);
  else if (level == Level::info)
  std::println("[info]  {}", msg);
  else if (level == Level::warn)
  std::println("[warn]  {}", msg);
  else
  assert(false);
}

template <typename... Args>
void log(Level level, std::format_string<Args...> fmt, Args&&... args)
{
  if (level == Level::error)
  std::println("[error] {}", fmt, std::forward<Args>(args)...);
  else if (level == Level::info)
  std::println("[info]  {}", fmt, std::forward<Args>(args)...);
  else if (level == Level::warn)
  std::println("[warn]  {}", fmt, std::forward<Args>(args)...);
  else
  assert(false);
}

template <typename T>
inline void error(T const& msg)
{
  log(Level::error, msg);
}

template <typename... Args>
inline void error(std::format_string<Args...> fmt, Args&&... args)
{
  log(Level::error, std::format(fmt, std::forward<Args>(args)...));
}

template <typename T>
inline void info(T const& msg)
{
  log(Level::info, msg);
}

template <typename... Args>
inline void info(std::format_string<Args...> fmt, Args&&... args)
{
  log(Level::info, std::format(fmt, std::forward<Args>(args)...));
}

template <typename T>
inline void warn(T const& msg)
{
  log(Level::warn, msg);
}

template <typename... Args>
inline void warn(std::format_string<Args...> fmt, Args&&... args)
{
  log(Level::warn, std::format(fmt, std::forward<Args>(args)...));
}

}}