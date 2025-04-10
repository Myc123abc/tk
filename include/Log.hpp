//
// log system
//
// only have control log function.
//
// TODO:
// expand other functions like file log.
//

#pragma once

#include <spdlog/spdlog.h>

#include <format>
#include <cassert>

namespace tk
{
  namespace log
  {
    class Log
    {
      static Log& instance() noexcept
      {
        static Log log;
        return log;
      }

      Log()
      {
        spdlog::set_pattern("%^%L:%$ %v");
      }

      enum class Level
      {
        error,
        info,
      };

      template <typename T>
      void log(Level level, T const& msg)
      {
        if (level == Level::error)
          spdlog::error(msg);
        else if (level == Level::info)
          spdlog::info(msg);
        else
          assert(false);
      }

      template <typename... Args>
      void log(Level level, std::format_string<Args...> fmt, Args&&... args)
      {
        if (level == Level::error)
          spdlog::error(fmt, std::forward<Args>(args)...);
        else if (level == Level::info)
          spdlog::info(fmt, std::forward<Args>(args)...);
        else
          assert(false);
      }

      template <typename T>
      friend void error(T const& msg);
      template <typename... Args>
      friend void error(std::format_string<Args...> fmt, Args&&... args);
      template <typename T>
      friend void info(T const& msg);
      template <typename... Args>
      friend void info(std::format_string<Args...> fmt, Args&&... args);
    };

    template <typename T>
    inline void error(T const& msg)
    {
      Log::instance().log(Log::Level::error, msg);
    }

    template <typename... Args>
    inline void error(std::format_string<Args...> fmt, Args&&... args)
    {
      Log::instance().log(Log::Level::error, fmt, std::forward<Args>(args)...);
    }

    template <typename T>
    inline void info(T const& msg)
    {
      Log::instance().log(Log::Level::info, msg);
    }

    template <typename... Args>
    inline void info(std::format_string<Args...> fmt, Args&&... args)
    {
      Log::instance().log(Log::Level::info, fmt, std::forward<Args>(args)...);
    }

  }
}
