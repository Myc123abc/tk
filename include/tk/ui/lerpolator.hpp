#pragma once

#include <algorithm>

namespace tk { namespace ui {

class Lerpolator
{
public:
  Lerpolator()                             = default;
  ~Lerpolator()                            = default;
  Lerpolator(Lerpolator const&)            = delete;
  Lerpolator(Lerpolator&&)                 = delete;
  Lerpolator& operator=(Lerpolator const&) = delete;
  Lerpolator& operator=(Lerpolator&&)      = delete;

  void init(double dur) noexcept
  {
    _dur = dur;
  }

  void start() noexcept
  {
    _state = State::started;
  }

  void reset() noexcept
  {
    _state    = State::not_started;
    _x        = 0.f;
    _reversed = false;
  }

  void update(double delta) noexcept
  {
    if (_state == State::started)
    {
      auto dx = delta / _dur;
      _x += _reversed ? -dx : dx;
      _x = std::clamp(_x, 0.0, 1.0);
      if (_x == 1.f || _reversed && _x == 0.f)
        _state = State::finished;
    }
  }

  void reverse() noexcept
  {
    _reversed = !_reversed;
    if (_state == State::finished) _state = State::not_started;
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
};

}}
