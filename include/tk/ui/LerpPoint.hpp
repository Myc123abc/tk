//
// simple lerp animation of point
//

#pragma once

#include <glm/glm.hpp>
#include <chrono>
#include <vector>

namespace tk { namespace ui {

struct LerpContext
{

};

class LerpPoint
{
public:
  LerpPoint(glm::vec2 start, glm::vec2 end, uint32_t time)
    : _start(start), _end(end), _now(start), _time(time) {}

  enum class status
  {
    unrun,
    running,
  };

  void run();
  void render();

  void reverse() noexcept { std::swap(_start, _end); }

  auto& start() const noexcept { return _start; }
  auto& end()   const noexcept { return _end;   }
  auto& now()   const noexcept { return _now;   }

  operator glm::vec2() const noexcept { return _now; }

private:
  glm::vec2 _start, _end, _now;
  uint32_t  _time; // millisecond
  status    _status = status::unrun;
  double    _rate   = {};
  decltype(std::chrono::high_resolution_clock::now()) _start_time;

  bool                  _reentry = {};
  glm::vec2             _reentry_start, _reentry_end;
  uint32_t              _reentry_time;
};

struct LerpInfo
{
  glm::vec2 start;
  glm::vec2 end;
  uint32_t  time;
};

class LerpPoints
{
public:
  LerpPoints(std::vector<LerpInfo> const& infos)
    :_infos(infos), _now(infos.begin()->start), _it(_infos.begin()) {}

  void run();
  void render();

  auto& start() const noexcept { return _infos.begin()->start; }
  auto& end()   const noexcept { return _infos.rbegin()->end;  }
  auto& now()   const noexcept { return _now; }

private:
  enum class status
  {
    unrun,
    running,
  };

  glm::vec2                _now;
  std::vector<LerpInfo>    _infos;
  decltype(_infos.begin()) _it;
  status                   _status = status::unrun;
  decltype(std::chrono::high_resolution_clock::now()) _start_time;
  double                   _rate = {};

  bool      _reentry       = {};
  glm::vec2 _reentry_start, _reentry_end;
  uint32_t  _renetry_time  = {};
};

}}