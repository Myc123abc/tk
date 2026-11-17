module tk.ui:ui.util;

import :transform;
import tk.ui.context;

namespace tk::ui::detail {

template <typename... Args>
void adjust_pos(Args&... args) noexcept
{
  auto offset = g_ctx.get_render_pos();
  ((args += offset), ...);
  auto scale = g_ctx.window()->scale();
  ((args *= scale), ...);
}

template <typename... Args>
void adjust_scale(Args&... args) noexcept
{
  auto scale = g_ctx.window()->scale();
  ((args *= scale), ...);
}

void adjust_transform(Matrix& transform) noexcept
{
  auto offset = g_ctx.get_render_pos();
  auto scale  = g_ctx.window()->scale();
  auto inv    = 1.f / scale;

  auto to_window   = Matrix{ scale, 0, 0, scale, offset.x * scale, offset.y * scale };
  auto from_window = Matrix{ inv, 0, 0, inv, -offset.x, -offset.y };
  transform = from_window * transform * to_window;
}

auto get_rect(float2 pos, float2 extent) noexcept -> Rect
{
  auto left_top     = pos;
  auto right_bottom = pos + extent;
  adjust_pos(left_top, right_bottom);
  return { left_top, right_bottom };
}

}
