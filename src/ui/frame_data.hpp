#pragma once

#include "ui/ui.hpp"
#include "../renderer/resource/shader_type.hpp"
#include "image_manager.hpp"

namespace tk::ui {

struct WindowShadowInfo
{
  RECT                scissor_rect;
  vec2u               window_extent{};
  float               shadow_thickness{};
  vec3                color{};
  float               radius{};
  float               softness{};
  std::optional<vec4> wireframe_color;
};

class FrameData
{
public:
  FrameData()                            = default;
  ~FrameData()                           = default;
  FrameData(FrameData const&)            = delete;
  FrameData(FrameData&&)                 = delete;
  FrameData& operator=(FrameData const&) = delete;
  FrameData& operator=(FrameData&&)      = delete;

  static void init() noexcept;

  void clear() noexcept
  {
    _vertices.clear();
    _indices.clear();
    _draw_datas.clear();
    _draw_data_rect_idxs.clear();
    _points.clear();
    _normals.clear();
    _vertex_beg         = {};
    _index_beg          = {};
    _index              = {};
    _tmp_vertices_size  = {};
    _tmp_indices_size   = {};
    _draw_index_beg     = {};
    _window_pos         = {};
    _window_shadow_info = {};
  }

  void add_rect(vec2 left_top, vec2 right_bottom, Color color, float thickness) noexcept;
  void add_triangle(vec2 p0, vec2 p1, vec2 p2, Color color, float thickness) noexcept;
  void draw_circle(vec2 center, float radius, Color color, float thickness) noexcept;
  void add_image(ImageHandle handle, vec2 left_top, vec2 right_bottom, uint8_t alpha) noexcept;

  void add_scissor_rect(RECT rect) noexcept;

  void set_window_pos(vec2 pos) noexcept { _window_pos = pos; }
  auto window_pos() const noexcept { return _window_pos; }

  void set_window_shadow(RECT scissor_rect, vec2u window_extent, float shadow_thickness, Color color, float radius, float softness, std::optional<vec4> wireframe_color) noexcept
  {
    _window_shadow_info = { scissor_rect, window_extent, shadow_thickness, { color.r, color.g, color.b }, radius, softness, wireframe_color };
  }
  auto& window_shadow_info() const noexcept { return _window_shadow_info; }

  auto& vertices()   const noexcept { return _vertices;   }
  auto& indices()    const noexcept { return _indices;    }
  auto& draw_datas() const noexcept { return _draw_datas; }

  auto check() const noexcept { return _draw_data_rect_idxs.empty(); }

private:
  using Vertex = renderer::Vertex;

  auto expand_beg(uint32_t vertices_size, uint32_t indices_size) noexcept -> std::pair<Vertex*, uint16_t*>
  {
    _tmp_vertices_size = vertices_size;
    _tmp_indices_size  = indices_size;

    _vertices.resize(_vertices.size() + vertices_size);
    _indices.resize(_indices.size() + indices_size);

    return { _vertices.data() + _vertex_beg, _indices.data() + _index_beg };
  }

  void expand_end() noexcept
  {
    _vertex_beg += _tmp_vertices_size;
    _index_beg  += _tmp_indices_size;
    _index      += _tmp_vertices_size;
  }

  struct DrawData
  {
    enum class Type
    {
      shape,
      image,
    };

    DrawData(Type type, uint32_t index_beg, uint32_t indices_size, ImageHandle image_handle = {}, uint8_t image_alpha = {})
      : type(type), index_beg(index_beg), indices_size(indices_size)
    {
      if (image_alpha)
      {
        this->image_handle = image_handle;
        this->image_alpha  = static_cast<float>(image_alpha) / 255;
      }
    }

    Type type{};

    RECT     scissor_rect{};
    uint32_t index_beg{};
    uint32_t indices_size{};

    ImageHandle image_handle{};
    float       image_alpha{};
  };

public:
  using DrawDataType = DrawData::Type;
private:
  void add_draw_call(DrawDataType type, ImageHandle image_handle = {}, uint8_t image_alpha = {}) noexcept;

  void set_points(std::vector<vec2>&& points) noexcept { _points = std::move(points); }
  void add_convex_poly_filled(Color color) noexcept;

  static auto get_circle_segment_count(float radius) noexcept -> uint32_t;
  void path_arc_to(vec2 center, float radius) noexcept;

private:
  std::vector<Vertex>   _vertices;
  std::vector<uint16_t> _indices;
  uint32_t              _vertex_beg{};
  uint32_t              _index_beg{};
  uint16_t              _index{};

  uint32_t              _tmp_vertices_size{};
  uint32_t              _tmp_indices_size{};

  std::vector<DrawData> _draw_datas;
  std::vector<uint32_t> _draw_data_rect_idxs;
  uint32_t              _draw_index_beg{};

  vec2                  _window_pos{};

  std::optional<WindowShadowInfo> _window_shadow_info;

  std::vector<vec2> _points;
  std::vector<vec2> _normals;

  inline static auto constexpr arc_table_size = 48;
  inline static auto constexpr arc_sample_max = arc_table_size;
  inline static std::array<int, 64> _circle_segment_counts;
  inline static std::array<vec2, arc_table_size> _arc_vertices;
};

using DrawDataType = FrameData::DrawDataType;

}
