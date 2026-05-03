#pragma once

#include "ui/ui.hpp"
#include "../renderer/resource/shader_type.hpp"
#include "image_manager.hpp"

namespace tk::ui {

struct WindowShadowInfo
{
  RECT                scissor_rect;
  uint2               window_extent{};
  float               shadow_thickness{};
  float3              color{};
  float               radius{};
  float               softness{};
  std::optional<float4> wireframe_color;
};

inline ImageHandle Write_Image_Handle = {};

enum class DrawCmdType
{
  ui,
  clear_mask_image,
  clear_tmp_image,
  mask_write,
  discard_draw_tmp,
  composite_tmp,
};

struct DrawCmd
{
  DrawCmdType type;

  union
  {
    struct
    {
      uint32_t    idx_beg{};
      uint32_t    idx_size{};
      RECT        scissor_rect{};
      ImageHandle image_handle;
    } ui;

    std::optional<RECT> clear_rect{};
  };
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
    _draw_cmds.clear();
    _draw_cmd_rect_idxs.clear();
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

  void add_rect(float2 left_top, float2 right_bottom, Color color, float thickness) noexcept;
  void add_triangle(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept;
  void add_circle(float2 center, float radius, Color color, float thickness) noexcept;
  void add_line(float2 p0, float2 p1, Color color, float thickness) noexcept;
  void add_bezier_quad(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept;
  void add_bezier_cubic(float2 p0, float2 p1, float2 p2, float2 p3, Color color, float thickness) noexcept;
  void add_image(ImageHandle handle, float2 left_top, float2 right_bottom, uint8_t alpha) noexcept;

  void path_line_to(float2 p) noexcept { _points.emplace_back(p); }
  void path_arc_to(float2 center, float radius, float min, float max) noexcept;
  void path_bezier_quad_to(float2 p1, float2 p2) noexcept;
  void path_bezier_cubic_to(float2 p1, float2 p2, float2 p3) noexcept;
  void path_end(Color color, float thickness, bool is_closed) noexcept;

  void add_scissor_rect(RECT rect) noexcept;

  void discard_beg(std::function<void()> func) noexcept;
  void discard_end() noexcept;
  void union_beg() noexcept;
  void union_end() noexcept;

  void set_window_pos(float2 pos) noexcept { _window_pos = pos; }
  auto window_pos() const noexcept { return _window_pos; }

  void set_window_shadow(RECT scissor_rect, uint2 window_extent, float shadow_thickness, Color color, float radius, float softness, std::optional<float4> wireframe_color) noexcept
  {
    _window_shadow_info = { scissor_rect, window_extent, shadow_thickness, { color.r, color.g, color.b }, radius, softness, wireframe_color };
  }
  auto& window_shadow_info() const noexcept { return _window_shadow_info; }

  auto& vertices()  const noexcept { return _vertices;  }
  auto& indices()   const noexcept { return _indices;   }
  auto& draw_cmds() const noexcept { return _draw_cmds; }

  auto check() const noexcept { return _draw_cmd_rect_idxs.empty(); }

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
  void add_rect(float2 left_top, float2 right_bottom, Color color = {}) noexcept;

  void push_draw_cmd(DrawCmdType type, ImageHandle image_handle = Write_Image_Handle) noexcept;
  void push_draw_cmd_clear_rect(DrawCmdType type, std::optional<RECT> rect = {}) noexcept;

  void add_convex_poly_filled(Color color) noexcept;
  void add_concave_poly_filled(Color color) noexcept;

  static auto calc_circle_segment_count(float radius) noexcept -> float;
  static auto get_circle_segment_count(float radius) noexcept -> uint32_t;

  void add_poly_line(Color color, float thickness, bool is_closed) noexcept;
  void _path_arc_to(float2 center, float radius, int min, int max) noexcept;
  void _path_arc_to(float2 center, float radius, int min, int max, int segment_cnt) noexcept;

  void path_bezier_quad_curve_to_casteljau(float2 p0, float2 p1, float2 p2, float tess_tol, int level) noexcept;
  void path_bezier_cubic_curve_to_casteljau(float2 p0, float2 p1, float2 p2, float2 p3, float tess_tol, int level) noexcept;

private:
  std::vector<Vertex>   _vertices;
  std::vector<uint16_t> _indices;
  uint32_t              _vertex_beg{};
  uint32_t              _index_beg{};

  uint32_t              _tmp_vertices_size{};
  uint32_t              _tmp_indices_size{};

  std::vector<DrawCmd>  _draw_cmds;
  std::vector<uint32_t> _draw_cmd_rect_idxs;
  uint32_t              _draw_index_beg{};

  float2                _window_pos{};

  std::optional<WindowShadowInfo> _window_shadow_info;

  std::vector<float2>   _points;
  std::vector<float2>   _normals;
  std::vector<float2>   _tmp_buf;

  bool     _using_discard_shapes{};
  bool     _use_discard{};
  uint32_t _discard_vtx_beg{};

  inline static auto constexpr arc_table_size         = 48;
  inline static auto constexpr arc_sample_max         = arc_table_size;
  inline static auto constexpr curve_tessellation_tol = 1.25f;
  inline static auto constexpr tessellation_max_error = 0.3f;
  inline static auto           arc_radius_cutoff      = 0.f;
  inline static std::array<int, 64>              _circle_segment_counts;
  inline static std::array<float2, arc_table_size> _arc_vertices;
};

}
