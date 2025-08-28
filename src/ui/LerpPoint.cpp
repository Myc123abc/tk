#include "tk/ui/LerpPoint.hpp"
#include "tk/util.hpp"

using namespace tk::ui;

void LerpPoint::run()
{
  using enum Status;
  if (_status == unrun)
  {
    _status     = running;
    _start_time = std::chrono::high_resolution_clock::now();
    if (_now == _end)
      std::swap(_start, _end);
  }
  else if (_status == running)
  {
    _reentry       = true;
    _start_time    = std::chrono::high_resolution_clock::now();
    _reentry_start = _now;
    _reentry_end   = _start;
    _reentry_time  = _time * glm::distance(_now, _start) / _distance;
    std::swap(_start, _end);
  }
  else
    assert(false);
}

void LerpPoint::render()
{
  using enum Status;
  if (_status == running)
  {
    if (_reentry)
    {
      auto duration = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - _start_time).count());
      if (_reentry_time == 0)
        _rate = 1.0;
      else
        _rate = glm::clamp(duration / _reentry_time, 0.0, 1.0);
      _now = util::lerp(_reentry_start, _reentry_end, _rate);
      if (_rate == 1.0)
      {
        _status        = unrun;
        _reentry       = false;
      }
    }
    else
    {
      auto duration = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - _start_time).count());
      _rate = glm::clamp(duration / _time, 0.0, 1.0);
      _now = util::lerp(_start, _end, _rate);
      if (_rate == 1.0)
      {
        _status = unrun;
      }
    }
  }
}

void LerpPoints::run()
{
  using enum Status;
  if (_status == unrun)
  {
    _status     = running;
    _start_time = std::chrono::high_resolution_clock::now();
    if (_it == _infos.end())
    {
      for (uint32_t i = 0, j = _infos.size() - 1; i < _infos.size() / 2; ++i, --j)
      {
        std::swap(_infos[i].start, _infos[i].end);
        std::swap(_infos[j].start, _infos[j].end);
        std::swap(_infos[i], _infos[j]);
      }
      _it = _infos.begin();
    }
  }
  else if (_status == running)
  {
    _start_time    = std::chrono::high_resolution_clock::now();
    _reentry       = true;
    _reentry_start = _now;
    _reentry_end   = _it->start;
    _reentry_time  = _it->time * glm::distance(_now, _it->start) / _it->distance;

    for (uint32_t i = 0, j = _infos.size() - 1; i < _infos.size() / 2; ++i, --j)
    {
      std::swap(_infos[i].start, _infos[i].end);
      std::swap(_infos[j].start, _infos[j].end);
      std::swap(_infos[i], _infos[j]);
    }
    
    _it = _infos.end() - (_it - _infos.begin()) - 1;
  }
  else
    assert(false);
}

void LerpPoints::render()
{
  using enum Status;
  if (_status == running)
  {
    if (_it == _infos.end())
    {
      _status = unrun;
      return;
    }

    if (_reentry)
    {
      auto duration = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - _start_time).count());
      if (_reentry_time == 0)
        _rate = 1.0;
      else
        _rate  = glm::clamp(duration / _reentry_time, 0.0, 1.0);
      _now   = util::lerp(_reentry_start, _reentry_end, _rate);
      _stage = _it->stage;
      if (_rate == 1.0)
      {
        _start_time    = std::chrono::high_resolution_clock::now();
        _reentry       = false;
        ++_it;
      }
    }
    else
    {
      auto duration = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - _start_time).count());
      _rate  = glm::clamp(duration / _it->time, 0.0, 1.0);
      _now   = util::lerp(_it->start, _it->end, _rate);
      _stage = _it->stage;
      if (_rate == 1.0)
      {
        _start_time = std::chrono::high_resolution_clock::now();
        ++_it;
      }
    }
  }
}