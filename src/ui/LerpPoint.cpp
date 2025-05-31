#include "tk/ui/LerpPoint.hpp"
#include "tk/util.hpp"

using namespace tk::ui;

void LerpPoint::run()
{
  assert(_time);
  using enum status;
  if (_status == unrun)
  {
    _status = running;
    _start_time = std::chrono::high_resolution_clock::now();
  }
  else if (_status == running)
  {
    _reentry            = true;
    _reentry_start_time = std::chrono::high_resolution_clock::now();
    _reentry_start      = _now;
    _reentry_time       = _rate * _time;
    _reentry_end        = ++_reentry_count % 2 ? _start : _end;
  }
  else
    assert(false);
}

void LerpPoint::render()
{
  using enum status;
  assert(_time);
  if (_status == running)
  {
    if (_reentry)
    {
      auto duration = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - _reentry_start_time).count());
      _rate = glm::clamp(duration / _reentry_time, 0.0, 1.0);
      _now = util::lerp(_reentry_start, _reentry_end, _rate);
      if (_rate == 1.0)
      {
        _status        = unrun;
        _reentry       = false;
        _reentry_count = {};
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