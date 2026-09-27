#pragma once

#include "common.hpp"
#include "tk/base.hpp"

#include <functional>
#include <algorithm>

namespace tk::ui {

class Tween
{
public:
  Tween()                        = default;
  ~Tween()                       = default;
  Tween(Tween const&)            = delete;
  Tween(Tween&&)                 = delete;
  Tween& operator=(Tween const&) = delete;
  Tween& operator=(Tween&&)      = delete;

  enum class Mode
  {
    none,
    loop,
  };

  static double linear(double x) noexcept { return x; }

  using Ease = std::function<double(double)>;

  auto init(double forward_dur, double reverse_dur, Mode mode = {}, Ease ease = linear) noexcept -> Tween&
  {
    _forward_dur = forward_dur;
    _reverse_dur = reverse_dur;
    _mode        = mode;
    _ease        = ease ? ease : linear;
    if (mode == Mode::loop) start();
    return *this;
  }

  auto init(double dur, Mode mode = {}, Ease ease = linear) noexcept -> Tween&
  {
    return init(dur, dur, mode, ease);
  }

  auto start() noexcept -> Tween&
  {
    _state = State::started;
    return *this;
  }

  auto reset() noexcept -> Tween&
  {
    _state    = State::not_started;
    _y        = 0.0;
    _x        = 0.0;
    _reversed = false;
    return *this;
  }

  auto update(double delta = ui::delta_time()) noexcept -> Tween&
  {
    if (_state == State::started)
    {
      auto dur = _reversed ? _reverse_dur : _forward_dur;
      auto dx  = dur ? (delta / dur) : 1.0;
      _x += _reversed ? -dx : dx;
      _x = std::clamp(_x, 0.0, 1.0);
      _y = _ease(_x);

      if (_x == 1.0 || _reversed && _x == 0.0)
      {
        if (_mode == Mode::loop)
          _x = _reversed ? 1.0 : 0.0;
        else
          _state = State::finished;
      }
    }
    return *this;
  }

  auto reverse() noexcept -> Tween&
  {
    _reversed = !_reversed;
    if (_state == State::finished) _state = State::not_started;
    return *this;
  }

  auto is_not_started() const noexcept { return _state == State::not_started; }
  auto is_started()     const noexcept { return _state == State::started;     }
  auto is_finished()    const noexcept { return _state == State::finished;    }
  auto is_reversed()    const noexcept { return _reversed;                    }
  auto get()            const noexcept { return _y;                           }

private:
  double _forward_dur{};
  double _reverse_dur{};
  double _x{};
  double _y{};
  Ease   _ease;

  enum class State
  {
    not_started,
    started,
    finished,
  };
  State _state{};
  bool  _reversed{};

  Mode _mode{};
};

class LoopTrigger
{
public:
  LoopTrigger()                              = default;
  ~LoopTrigger()                             = default;
  LoopTrigger(LoopTrigger const&)            = delete;
  LoopTrigger(LoopTrigger&&)                 = delete;
  LoopTrigger& operator=(LoopTrigger const&) = delete;
  LoopTrigger& operator=(LoopTrigger&&)      = delete;

  void init(uint dur, bool b) noexcept
  {
    _dur  = dur;
    _time = {};
    _b    = b;
  }

  void update(double delta = ui::delta_time()) noexcept
  {
    _time += delta; 
    if (_time > _dur)
    {
      _b    = !_b;
      _time = {};
    }
  }

  constexpr operator bool() const noexcept { return _b; }

private:
  uint   _dur{};
  double _time{};
  bool   _b{};
};

}
