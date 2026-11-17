module;

#include <functional>
#include <optional>
#include <utility>

module tk.ui;

import :window;
import tk.base;
import tk.renderer;
import tk.string_id;
import tk.ui.config;
import tk.ui.context;

namespace tk::ui {

auto get_cursor_pos() noexcept -> float2
{
  return renderer::get_cursor_pos();
}

auto get_cursor_pos_on_window() noexcept -> float2
{
  g_ctx.check_draw();
  return g_ctx.cursor_pos_on_window();
}

void begin(StringID name, std::optional<float2> extent, std::optional<float2> pos, bool* is_closed, WindowConfig const& cfg) noexcept
{
  g_ctx.begin(name, extent, pos, is_closed, cfg);
}

void end() noexcept
{
  g_ctx.end();
}

void add_move_invalid_area(float2 left_top, float2 right_bottom) noexcept
{
  g_ctx.check_draw();
  g_ctx.add_move_invalid_area({ left_top, right_bottom });
}

auto window_extent() noexcept -> float2
{
  g_ctx.check_draw();
  return float2{ g_ctx.window()->width(), g_ctx.window()->height() } / g_ctx.window()->scale();
}

auto window_pos() noexcept -> float2
{
  g_ctx.check_draw();
  return g_ctx.window()->pos();
}

auto window_drawable_extent() noexcept -> float2
{
  auto extent = window_extent();
  if (g_ctx.is_use_title_bar_now())
    extent.y -= Title_Bar_Height;
  return extent;
}

auto is_maximized_window() noexcept -> bool
{
  g_ctx.check_draw();
  return g_ctx.window()->is_maximized();
}

void maximize_window() noexcept
{
  g_ctx.check_draw();
  if (g_ctx.window()->cfg().no_resize) return;
  g_ctx.window()->maximize();
}

auto is_fullscreen_window() noexcept -> bool
{
  g_ctx.check_draw();
  return g_ctx.window()->is_fullscreen();
}

void fullscreen_window() noexcept
{
  g_ctx.check_draw();
  if (g_ctx.window()->cfg().no_resize) return;
  g_ctx.fullscreen_window();
}

void restore_fullscreen_window() noexcept
{
  g_ctx.check_draw();
  if (g_ctx.window()->cfg().no_resize) return;
  g_ctx.restore_fullscreen_window();
}

void draw_on_title_bar(std::move_only_function<void(float2)>&& func) noexcept
{
  g_ctx.check_draw();
  g_ctx.draw_on_title_bar(std::move(func));
}

}
