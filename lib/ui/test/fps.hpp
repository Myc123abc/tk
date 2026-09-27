#pragma once

#include "tk/base.hpp"
#include "tk/ui/ui.hpp"

namespace tk {

class Fps
{
public:
  void init(int target_fps) noexcept
  {
    _target_fps        = target_fps;
    _target_frame_time = (1000'000 / target_fps) + 1;
    QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&_frequency));
  }

  void update() noexcept
  {
    if (_target_fps > 0)
      fps_limit();
    calc_fps();
  }

  auto get() const noexcept { return _fps; }

  void start() noexcept
  {
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&_frame_beg));
  }

private:
  void calc_fps() noexcept
  {
    // delta unit change to sec
    auto delta = ui::delta_time() / 1000'000;

    // calc accum
    _accum += delta - _deltas[_idx];

    // store delta
    _deltas[_idx] = delta;

    // move to next
    _idx = (_idx + 1) % _countof(_deltas);

    // get delta cnt
    _cnt = std::min(_cnt + 1, static_cast<uint>(_countof(_deltas)));

    // calc fps
    _fps = _accum > 0.f ? 1.f / (_accum / _cnt) : std::numeric_limits<float>::max();
  }

  auto elapsed_time_micro() const noexcept
  {
    auto cnt = _frame_end - _frame_beg;
    cnt *= 1000'000;
    cnt /= _frequency;
    return cnt;
  }

  void fps_limit() noexcept
  {
    QueryPerformanceCounter((LARGE_INTEGER*)&_frame_end);
    auto elapsed_time = elapsed_time_micro();
    while (elapsed_time < _target_frame_time)
    {
      if ((elapsed_time + _over_sleep_dur) >= _target_frame_time)
      {
        _over_sleep_dur -= _target_frame_time - elapsed_time;
        break;
      }

      Sleep(1);

      QueryPerformanceCounter((LARGE_INTEGER*)&_frame_end);
      elapsed_time = elapsed_time_micro();

      if (elapsed_time > _target_frame_time)
        _over_sleep_dur += elapsed_time - _target_frame_time;
    }

    QueryPerformanceCounter((LARGE_INTEGER*)&_frame_end);
    _ticks_accumulator += _frame_end - _frame_beg;
    ++_frame_cnt;

    if ((_frame_cnt % _target_fps) == 0)
    {
      _average_fps = ((_frequency * _target_fps) + (_ticks_accumulator - 1)) / _ticks_accumulator;
      _ticks_accumulator = 0;
      _frame_cnt         = 0;
    }

    _frame_beg = _frame_end;
  }

private:
  float  _fps{};
  float  _deltas[60]{};
  uint   _idx{};
  float  _accum{};
  uint   _cnt{};

  int    _target_fps{};
  uint   _target_frame_time{};
  uint64 _frequency{};
  int32  _frame_cnt;
  int64  _frame_beg;
  int64  _frame_end;
  int64  _average_fps;
  int64  _ticks_accumulator;
  int64  _over_sleep_dur;
};

}
