module;

#include <cmath>

module tk.ui;

import :animation;
import tk.base;
import tk.string_id;
import tk.ui.context;

namespace tk::ui {

auto lerp(Color x, Color y, float v) noexcept -> float4
{
  return
  {
    std::lerp(x.r, y.r, v),
    std::lerp(x.g, y.g, v),
    std::lerp(x.b, y.b, v),
    std::lerp(x.a, y.a, v)
  };
}

auto lerp(float2 x, float2 y, float v) noexcept -> float2
{
  return { std::lerp(x.x, y.x, v), std::lerp(x.y, y.y, v) };
}

auto ping_pong(StringID name, bool b, double forward_dur, double reverse_dur, Tween::Ease ease) noexcept -> double
{
  return g_ctx.ping_pong(b, g_ctx.generic_id(name), forward_dur, reverse_dur, ease);
}

void reset_tween(StringID name) noexcept
{
  g_ctx.reset_tween(g_ctx.get_id(name));
}

}
