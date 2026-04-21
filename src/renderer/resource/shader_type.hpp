#pragma once

#include "util/vec.hpp"

namespace tk::renderer {

// dx12 vertex is aligment at 8 bytes
// here use 16 bytes because vec4 is simd which need 16 bytes aligment
struct alignas(16) Vertex
{
  vec2 pos{};
  vec2 uv{};
  vec4 color{};
};

struct alignas(16) Constants
{
  vec2u    render_target_extent{};
  vec2u    window_extent{};

  vec2     window_pos{};
  float    shadow_thickness{};
  float    shadow_radius{};

  vec3     shadow_color{};
  float    shadow_softness{};

  vec4     wireframe_color{};

  uint32_t draw_wireframe{};
  float    image_alpha{};
};

constexpr auto Max_Blur_Widget_Num = 5;
struct alignas(16) BlurConstants
{
  uint32_t blur_radius{};
  vec3     padding{};
  float    widgets[Max_Blur_Widget_Num * 2 + 1]{};
};

}
