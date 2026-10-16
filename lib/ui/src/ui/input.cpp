#include "ui/input.hpp"
#include "ui_context.hpp"
#include "input_system.hpp"

namespace tk::ui {

auto GetKeyResult::is_uppercase() const noexcept -> bool
{
  if (is_caps_locked()) return !get_key(Key::Shift).has_down();
  return get_key(Key::Shift).has_down();
}

auto get_key(Key key) noexcept -> GetKeyResult
{
  g_ui_ctx.check_draw();
  if (g_ui_ctx.window()->is_active())
    return { g_input_sys[key] };
  return {};
}

auto get_mouse_wheel_delta() noexcept -> float
{
  g_ui_ctx.check_draw();
  if (g_ui_ctx.window()->is_active())
    return g_input_sys.mouse_wheel_delta();
  return {};
}

}
