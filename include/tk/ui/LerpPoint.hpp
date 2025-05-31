//
// simple lerp animation of point
//

#pragma once

#include <glm/glm.hpp>
#include <chrono>

namespace tk { namespace ui {

class LerpPoint
{
public:
  LerpPoint(glm::vec2 start, glm::vec2 end, uint32_t time = 0)
    : _start(start), _end(end), _now(start), _time(time) {}

  void set_time(uint32_t time) noexcept { _time = time; }

  enum class status
  {
    unrun,
    running,
  };

  void run();
  void render();

  void reverse() noexcept { std::swap(_start, _end); }

  auto start() const noexcept { return _start; }
  auto end()   const noexcept { return _end;   }
  auto now()   const noexcept { return _now;   }

private:
  glm::vec2 _start, _end, _now;
  uint32_t  _time; // millisecond
  status    _status = status::unrun;
  double    _rate   = {};
  decltype(std::chrono::high_resolution_clock::now()) _start_time;

  uint32_t              _reentry_count = {};
  bool                  _reentry       = {};
  glm::vec2             _reentry_start, _reentry_end;
  uint32_t              _reentry_time;
  decltype(_start_time) _reentry_start_time;
};

}}