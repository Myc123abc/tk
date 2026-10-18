#pragma once

#include "../util/singleton.hpp"
#include "tk/base.hpp"
#include "tk/ui/input.hpp"

#include <array>
#include <bitset>

namespace tk::ui {

Singleton(InputSystem, g_input_sys,
public:
  auto operator[](Key key) const noexcept -> Flag<KeyState>
  {
    return _keys[index(key)].state;
  }

  void update() noexcept;
  void release_all() noexcept;

  void key_down(Key key, bool repeat = false) noexcept;
  void key_up(Key key) noexcept;

  void add_mouse_move_delta(int2 delta) noexcept { _mouse_move_delta += delta; }
  auto mouse_move_delta() const noexcept { return _mouse_move_delta; }

  void add_mouse_wheel_delta(float delta) noexcept { _mouse_wheel_delta += delta; }
  auto mouse_wheel_delta() const noexcept { return _mouse_wheel_delta; }

private:
  struct KeyContext
  {
    KeyState state{};
    bool     release_pending{};
  };

  static constexpr auto Key_Count = size_t{ 256 };

  static constexpr auto index(Key key) noexcept -> size_t
  {
    return static_cast<size_t>(key);
  }

  std::array<KeyContext, Key_Count> _keys;
  std::bitset<Key_Count>            _active_keys;
  int2                              _mouse_move_delta{};
  float                             _mouse_wheel_delta{};
)

}
