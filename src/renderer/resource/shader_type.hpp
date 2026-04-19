#pragma once

#include <glm/glm.hpp>

namespace tk::renderer {

struct alignas(8) Vertex
{
  glm::vec2 pos{};
  glm::vec4 color{};
  glm::vec2 uv{};
};

struct alignas(16) Constants
{
  glm::vec<2, uint32_t> render_target_extent{};
  glm::vec<2, uint32_t> window_extent{};

  glm::vec2             window_pos{};
  float                 shadow_thickness{};
  float                 shadow_radius{};

  glm::vec3             shadow_color{};
  float                 shadow_softness{};

  glm::vec4             wireframe_color{};

  uint32_t              draw_wireframe{};
  float                 image_alpha{};
};

constexpr auto Max_Blur_Widget_Num = 5;
struct alignas(16) BlurConstants
{
  uint32_t  blur_radius{};
  glm::vec3 padding{};
  float     widgets[Max_Blur_Widget_Num * 2 + 1]{};
};

}
