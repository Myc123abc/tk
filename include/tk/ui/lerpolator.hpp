#pragma once

#include "ui.hpp"

#include <algorithm>

namespace tk::ui {

class Lerpolator
{
public:
  Lerpolator()                             = default;
  ~Lerpolator()                            = default;
  Lerpolator(Lerpolator const&)            = delete;
  Lerpolator(Lerpolator&&)                 = delete;
  Lerpolator& operator=(Lerpolator const&) = delete;
  Lerpolator& operator=(Lerpolator&&)      = delete;

  enum class Mode
  {
    none,
    loop,
  };

  auto init(double dur, Mode mode = {}) noexcept -> Lerpolator&
  {
    _dur  = dur;
    _mode = mode;
    if (mode == Mode::loop) start();
    return *this;
  }

  auto start() noexcept -> Lerpolator&
  {
    _state = State::started;
    return *this;
  }

  auto reset() noexcept -> Lerpolator&
  {
    _state    = State::not_started;
    _x        = 0.f;
    _reversed = false;
    return *this;
  }

  auto update(double delta = ui::delta_time()) noexcept -> Lerpolator&
  {
    if (_state == State::started)
    {
      auto dx = delta / _dur;
      _x += _reversed ? -dx : dx;
      _x = std::clamp(_x, 0.0, 1.0);
      if (_x == 1.f || _reversed && _x == 0.f)
      {
        if (_mode == Mode::loop)
          _x = _reversed ? 1.f : 0.f;
        else
          _state = State::finished;
      }
    }
    return *this;
  }

  auto reverse() noexcept -> Lerpolator&
  {
    _reversed = !_reversed;
    if (_state == State::finished) _state = State::not_started;
    return *this;
  }

  auto is_not_started() const noexcept { return _state == State::not_started; }
  auto is_started()     const noexcept { return _state == State::started;     }
  auto is_finished()    const noexcept { return _state == State::finished;    }
  auto is_reversed()    const noexcept { return _reversed;                    }
  auto get()            const noexcept { return _x;                           }

private:
  double _dur{};
  double _x{};

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

  void init(uint32_t dur, bool b) noexcept
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
  uint32_t _dur{};
  double   _time{};
  bool     _b{};
};

}
