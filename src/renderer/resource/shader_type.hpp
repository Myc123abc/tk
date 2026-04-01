#pragma once

#include <glm/glm.hpp>

#include <bit>

namespace tk::renderer {

struct alignas(8) Vertex
{
  glm::vec3 pos{};
  glm::vec2 uv{};
  uint32_t  buffer_offset{};
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

struct ShapeProperty
{
  enum class Type : uint32_t
  {
    triangle = 1,
    rectangle,
    circle,
    line,
    bezier,

    path,
    path_line,
    path_bezier,

    glyph,
  };

  enum class Operator : uint32_t
  {
    none,
    u,
    discard
  };

  enum class Flag : uint32_t
  {
  };

  struct Header
  {
    Type      type{};
    glm::vec4 color{};
    float     thickness{};
    Operator  op{};
    Flag      flags{};
  };

  ShapeProperty(Type type, glm::vec4 color = {}, float thickness = {}, Operator op = {}, std::vector<float> const& values = {}, Flag flags = {}) noexcept
  {
    _data.reserve(sizeof(Header) / sizeof(uint32_t) + values.size() * sizeof(float));
    _data.emplace_back(std::bit_cast<uint32_t>(type));
    _data.emplace_back(std::bit_cast<uint32_t>(color.r));
    _data.emplace_back(std::bit_cast<uint32_t>(color.g));
    _data.emplace_back(std::bit_cast<uint32_t>(color.b));
    _data.emplace_back(std::bit_cast<uint32_t>(color.a));
    _data.emplace_back(std::bit_cast<uint32_t>(thickness));
    _data.emplace_back(std::bit_cast<uint32_t>(op));
    _data.emplace_back(std::bit_cast<uint32_t>(flags));
    for (auto const& v : values)
      _data.emplace_back(std::bit_cast<uint32_t>(v));
  }

  auto data()      const noexcept { return _data.data();                    }
  auto byte_size() const noexcept { return _data.size() * sizeof(uint32_t); }

  void set_color(glm::vec4 color) noexcept
  { 
    _data[1] = std::bit_cast<uint32_t>(color.r);
    _data[2] = std::bit_cast<uint32_t>(color.g);
    _data[3] = std::bit_cast<uint32_t>(color.b);
    _data[4] = std::bit_cast<uint32_t>(color.a);
  }
  void set_thickness(float thickness) noexcept { _data[5] = std::bit_cast<uint32_t>(thickness); }
  void set_operator(Operator op)      noexcept { _data[6] = std::bit_cast<uint32_t>(op);        }
  void set_flags(Flag flags)          noexcept { _data[7] = std::bit_cast<uint32_t>(flags);     }

private:
  std::vector<uint32_t> _data{};
};

constexpr auto Max_Blur_Widget_Num = 5;
struct alignas(16) BlurConstants
{
  uint32_t  blur_radius{};
  glm::vec3 padding{};
  float     widgets[Max_Blur_Widget_Num * 2 + 1]{};
};

}
