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

#include <string_view>

namespace tk
{
  namespace log
  {
    class Log
    {
      friend void error(std::string_view msg);
      friend void info(std::string_view msg);

      static Log& instance() noexcept
      {
        static Log log;
        return log;
      }

      Log() = default;

      void error(std::string_view msg)
      {
        // fmt::print("[");
        // fmt::print(fg(fmt::color::red), "error");
        // fmt::print("] {}\n", msg);
      }

      void info(std::string_view msg)
      {
        // fmt::print("[");
        // fmt::print(fg(fmt::color::green), "info");
        // fmt::print("] {}\n", msg);
      }
    };

    /**
     * Log error message.
     *
     * @param msg error message.
     */
    inline void error(std::string_view msg)
    {
      Log::instance().error(msg);
    }

    /**
     * Log information.
     *
     * @param msg information.
     */
    inline void info(std::string_view msg)
    {
      Log::instance().info(msg);
    }

  }
}
