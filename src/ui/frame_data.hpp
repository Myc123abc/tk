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

inline ImageHandle Write_Image_Handle = {};

struct DrawData
{
  enum class Type
  {
    ui,
    mask_write,
    discard_draw_tmp,
    composite_tmp,
    stencil_replace_write,
    stencil_equal_test,
    stencil_not_equal_test,
  };

  DrawData(
    Type        type,
    uint32_t    index_beg,
    uint32_t    indices_size,
    ImageHandle image_handle        = {},
    uint32_t    stencil_value       = {},
    bool        clear_depth_stencil = {},
    bool        clear_render_target = {}) noexcept
    : type(type), index_beg(index_beg), indices_size(indices_size), 
      stencil_value(stencil_value), clear_depth_stencil(clear_depth_stencil),
      clear_render_target(clear_render_target)
  {
    if (image_handle)
      this->image_handle = image_handle;
    else
      this->image_handle = Write_Image_Handle;
  }

  Type type{};

  RECT     scissor_rect{};
  uint32_t index_beg{};
  uint32_t indices_size{};

  ImageHandle image_handle{};

  uint32_t            stencil_value{};
  bool                clear_depth_stencil{};
  bool                clear_render_target{};
  std::optional<RECT> clear_render_target_rect{};
};

using DrawDataType = DrawData::Type;

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
    _vertex_beg           = {};
    _index_beg            = {};
    _tmp_vertices_size    = {};
    _tmp_indices_size     = {};
    _draw_index_beg       = {};
    _window_pos           = {};
    _window_shadow_info   = {};
    _use_discard          = {};
    _using_discard_shapes = {};
    _discard_vtx_beg      = {};
  }

  void add_rect(vec2 left_top, vec2 right_bottom, Color color, float thickness) noexcept;
  void add_triangle(vec2 p0, vec2 p1, vec2 p2, Color color, float thickness) noexcept;
  void add_circle(vec2 center, float radius, Color color, float thickness) noexcept;
  void add_line(vec2 p0, vec2 p1, Color color, float thickness) noexcept;
  void add_bezier_quad(vec2 p0, vec2 p1, vec2 p2, Color color, float thickness) noexcept;
  void add_bezier_cubic(vec2 p0, vec2 p1, vec2 p2, vec2 p3, Color color, float thickness) noexcept;
  void add_image(ImageHandle handle, vec2 left_top, vec2 right_bottom, uint8_t alpha) noexcept;

  void path_line_to(vec2 p) noexcept { _points.emplace_back(p); }
  void path_arc_to(vec2 center, float radius, float min, float max) noexcept;
  void path_bezier_quad_to(vec2 p1, vec2 p2) noexcept;
  void path_bezier_cubic_to(vec2 p1, vec2 p2, vec2 p3) noexcept;
  void path_end(Color color, float thickness, bool is_closed) noexcept;

  void add_scissor_rect(RECT rect) noexcept;

  void discard_beg(std::function<void()> func) noexcept;
  void discard_end() noexcept;

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
  }

private:
  auto get_rect(uint32_t vtx_beg, uint32_t vtx_cnt) const noexcept -> RECT;
  void add_rect(vec2 left_top, vec2 right_bottom, Color color) noexcept;

  void add_draw_call(DrawDataType type, ImageHandle image_handle = {}, uint32_t stencil_value = {}, bool clear_depth_stencil = {}, bool clear_render_target = {}) noexcept;

  void add_convex_poly_filled(Color color) noexcept;
  void add_concave_poly_filled(Color color) noexcept;

  static auto calc_circle_segment_count(float radius) noexcept -> float;
  static auto get_circle_segment_count(float radius) noexcept -> uint32_t;

  void add_poly_line(Color color, float thickness, bool is_closed) noexcept;
  void _path_arc_to(vec2 center, float radius, int min, int max) noexcept;
  void _path_arc_to(vec2 center, float radius, int min, int max, int segment_cnt) noexcept;

  void path_bezier_quad_curve_to_casteljau(vec2 p0, vec2 p1, vec2 p2, float tess_tol, int level) noexcept;
  void path_bezier_cubic_curve_to_casteljau(vec2 p0, vec2 p1, vec2 p2, vec2 p3, float tess_tol, int level) noexcept;

private:
  std::vector<Vertex>   _vertices;
  std::vector<uint16_t> _indices;
  uint32_t              _vertex_beg{};
  uint32_t              _index_beg{};

  uint32_t              _tmp_vertices_size{};
  uint32_t              _tmp_indices_size{};

  std::vector<DrawData> _draw_datas;
  std::vector<uint32_t> _draw_data_rect_idxs;
  uint32_t              _draw_index_beg{};

  vec2                  _window_pos{};

  std::optional<WindowShadowInfo> _window_shadow_info;

  std::vector<vec2> _points;
  std::vector<vec2> _normals;
  std::vector<vec2> _tmp_buf;

  bool     _using_discard_shapes{};
  bool     _use_discard{};
  uint32_t _discard_vtx_beg{};

  inline static auto constexpr arc_table_size         = 48;
  inline static auto constexpr arc_sample_max         = arc_table_size;
  inline static auto constexpr curve_tessellation_tol = 1.25f;
  inline static auto constexpr tessellation_max_error = 0.3f;
  inline static auto           arc_radius_cutoff      = 0.f;
  inline static std::array<int, 64>              _circle_segment_counts;
  inline static std::array<vec2, arc_table_size> _arc_vertices;
};

}
