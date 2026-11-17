module;

#include <functional>

module tk.ui;

import :shape_operation;
import :ui.util;
import tk.base;
import tk.renderer;
import tk.string_id;
import tk.ui.context;

namespace tk::ui {

void transform_beg(Matrix const& transform) noexcept
{
  g_ctx.check_draw();
  g_ctx.custom_transform_beg();
  auto adjusted = transform;
  detail::adjust_transform(adjusted);
  g_ctx.frame_data()->transform_beg(adjusted);
}

void transform_beg(Transform const& transform) noexcept
{
  transform_beg(transform.matrix());
}

void transform_end() noexcept
{
  g_ctx.check_draw();
  g_ctx.custom_transform_end();
  g_ctx.frame_data()->transform_end();
}

void discard_beg(std::function<void()> func) noexcept
{
  g_ctx.check_draw();
  g_ctx.frame_data()->discard_beg(func);
}

void discard_end() noexcept
{
  g_ctx.check_draw();
  g_ctx.frame_data()->discard_end();
}

void scissor_area_begin(float2 pos, float2 extent) noexcept
{
  g_ctx.check_draw();
  g_ctx.scissor_area_begin(detail::get_rect(pos, extent));
}

void scissor_area_end() noexcept
{
  g_ctx.check_draw();
  g_ctx.scissor_area_end();
}

void viewport_begin(StringID name, float2 pos, float2 extent) noexcept
{
  g_ctx.check_draw();
  g_ctx.viewport_begin(name, detail::get_rect(pos, extent));
}

void viewport_end() noexcept
{
  g_ctx.check_draw();
  g_ctx.viewport_end();
}

}
