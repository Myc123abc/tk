#pragma once

#include "ui/ui.hpp"

#include <algorithm>

namespace tk { namespace ui {

// TODO: abstract base part
//       expand to point lerp
//       add custom interpolation function
class ColorLerpolator
{
public:
  ColorLerpolator()                                  = default;
  ~ColorLerpolator()                                 = default;
  ColorLerpolator(ColorLerpolator const&)            = delete;
  ColorLerpolator(ColorLerpolator&&)                 = delete;
  ColorLerpolator& operator=(ColorLerpolator const&) = delete;
  ColorLerpolator& operator=(ColorLerpolator&&)      = delete;

  void init(Color beg, Color end, double dur) noexcept
  {
    _beg = beg;
    _end = end;
    _dur = dur;
  }

  void start() noexcept
  {
    _state = State::started;
  }

  void reset() noexcept
  {
    _state = State::not_started;
    _x     = 0.f;
    if (_reversed)
      std::swap(_beg, _end);
    _reversed = false;
  }

  void update(double delta) noexcept
  {
    if (_state == State::started)
    {
      _x += delta / _dur;
      _x = std::clamp(_x, 0.f, 1.f);
      if (_x == 1.f)
        _state = State::finished;
    }
  }

  void reverse() noexcept
  {
    _reversed = !_reversed;
    std::swap(_beg, _end);
    switch (_state)
    {
    case State::not_started:
      break;

    case State::started:
      _x = 1.f - _x;
      break;

    case State::finished:
      _state = State::not_started;
      _x     = 0.f;
      break;
    }
  }

  auto get() const noexcept
  {
    return color_lerp(_beg, _end, _x);
  }

  auto is_started()     const noexcept { return _state == State::started;     }
  auto is_not_started() const noexcept { return _state == State::not_started; }
  auto is_finished()    const noexcept { return _state == State::finished;    }
  auto is_reversed()    const noexcept { return _reversed;                    }

private:
  Color  _beg;
  Color  _end;
  double _dur{};
  float  _x{};

  enum class State
  {
    not_started,
    started,
    finished,
  };
  State _state{};
  bool  _reversed{};
};

}}
