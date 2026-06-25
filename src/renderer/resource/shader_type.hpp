#pragma once

#include "util/base.hpp"

namespace tk::renderer {

enum class VtxType
{
  shape,
  image,
  text,
};
  
inline auto vtx_pack(VtxType type, uint img_idx) noexcept
{
  return static_cast<uint>(type) << 16 | img_idx;
}

struct alignas(16) Vertex
{
  float2 pos{};
  float2 uv{};
  uint   color{};
  uint   packed{};
  uint   outer_col{};
  float  outer_width{};
};

struct alignas(16) Constants
{
  uint2  render_target_extent;
  uint2  window_extent;

  float2 window_pos;
  float  shadow_thickness{};
  float  shadow_radius{};

  float3 shadow_color;
  float  shadow_softness{};

  float4 wireframe_color;

  float2 mask_offset;
  float2 mask_extent;

  float2 composite_offset;
  float2 composite_extent;

  uint   draw_wireframe{};
};

constexpr auto Max_Blur_Widget_Num = 5;
struct alignas(16) BlurConstants
{
  uint   blur_radius{};
  float3 padding{};
  float  widgets[Max_Blur_Widget_Num * 2 + 1]{};
};

}
