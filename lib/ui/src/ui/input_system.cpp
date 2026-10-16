#include "input_system.hpp"

namespace {

auto is_mouse_button(tk::ui::Key key) noexcept -> bool
{
  using enum tk::ui::Key;
  return key == Mouse_Left_Button   ||
         key == Mouse_Right_Button  ||
         key == Mouse_Middle_Button ||
         key == Mouse_X1_Button     ||
         key == Mouse_X2_Button;
}

}

namespace tk::ui {

void InputSystem::update() noexcept
{
  using enum KeyState;

  _mouse_move_delta  = {};
  _mouse_wheel_delta = {};

  for (auto i = 0; i < Key_Count; ++i)
  {
    if (!_active_keys[i]) continue;

    auto  key = static_cast<Key>(i);
    auto& ctx = _keys[i];
    if (ctx.release_pending)
    {
      ctx.state           = up;
      ctx.release_pending = false;
    }
    else if (ctx.state == down)
      ctx.state = is_mouse_button(key) ? press : down_idle;
    else if (ctx.state == press && !is_mouse_button(key))
      ctx.state = down_idle;
    else if (ctx.state == up || ctx.state == idle)
    {
      ctx = {};
      _active_keys.reset(i);
    }
  }
}

void InputSystem::key_down(Key key, bool repeat) noexcept
{
  using enum KeyState;

  auto i    = index(key);
  auto& ctx = _keys[i];

  ctx.state = repeat && (ctx.state == down_idle || ctx.state == press) ? press : down;
  ctx.release_pending = false;
  _active_keys.set(i);
}

void InputSystem::key_up(Key key) noexcept
{
  using enum KeyState;

  auto i = index(key);
  if (!_active_keys[i]) return;

  auto& ctx = _keys[i];
  if (ctx.state == down || (ctx.state == press && !is_mouse_button(key)))
    ctx.release_pending = true;
  else
  {
    ctx.state           = up;
    ctx.release_pending = false;
  }
}

void InputSystem::release_all() noexcept
{
  for (auto i = 0; i < Key_Count; ++i)
  {
    if (!_active_keys[i]) continue;

    _keys[i].state           = KeyState::up;
    _keys[i].release_pending = false;
  }
}

}
