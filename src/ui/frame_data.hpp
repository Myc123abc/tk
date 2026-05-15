#pragma once

#include "ui/ui.hpp"
#include "../renderer/resource/shader_type.hpp"
#include "image_manager.hpp"
#include "util/rect.hpp"

namespace tk::ui {

struct WindowShadowInfo
{
  Rect                 scissor_rect;
  uint2                window_extent{};
  float                shadow_thickness{};
  float3               color{};
  float                radius{};
  float                softness{};
  std::optional<float4> wireframe_color;
};

inline ImageHandle Write_Image_Handle = {};

enum class RenderCmdType
{
  ui,

  clear_discard_image,
  clear_composite_image,
  discard_write,
  discard_draw_composite,
  discard_composite,

  clear_union_image,
  union_write,
};

struct RenderCmd
{
  RenderCmdType type;

  union
  {
    struct
    {
      uint32_t    idx_beg{};
      uint32_t    idx_size{};
      Rect        scissor_rect{};
      ImageHandle image_handle;
    } ui;

    std::optional<Rect> clear_rect{};
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
    _draw_cmds.clear();
    _vertices.clear();
    _indices.clear();
    _render_cmds.clear();
    _render_cmd_rect_idxs.clear();
    _points.clear();
    _normals.clear();
    _vertex_beg                    = {};
    _index_beg                     = {};
    _tmp_vertices_size             = {};
    _tmp_indices_size              = {};
    _draw_index_beg                = {};
    _window_pos                    = {};
    _window_shadow_info            = {};
    _use_discard                   = {};
    _using_discard_shapes          = {};
    _discard_vtx_beg               = {};
    _clear_composite_image_cmd_idx = {};
  }

  struct DrawCmd
  {
    enum class Type
    {
      add_rect,
      add_triangle,
      add_circle,
      add_line,
      add_arc,
      add_quad_bezier,
      add_cubic_bezier,
      add_image,
      path_begin,
      add_path_line_to,
      add_path_arc_to,
      add_path_quad_bezier_to,
      add_path_cubic_bezier_to,
      path_end,
      add_scissor_rect,
      discard_beg,
      discard_end,
      union_beg,
      union_end,
    } type{};

    struct
    {
      float2 left_top{};
      float2 right_bottom{};
      Color  color{};
      float  thickness{};
    } add_rect{};

    struct
    {
      float2 p0{};
      float2 p1{};
      float2 p2{};
      Color  color{};
      float  thickness{};
    } add_triangle{};

    struct
    {
      float2 center{};
      float  radius{};
      Color  color{};
      float  thickness{};
    } add_circle{};

    struct
    {
      float2 p0{};
      float2 p1{};
      Color  color{};
      float  thickness{};
    } add_line{};

    struct
    {
      float2 center{};
      float2 p0{};
      float2 p1{};
      Color  color{};
      float  thickness{};
    } add_arc{};

    struct
    {
      float2 p0{};
      float2 p1{};
      float2 p2{};
      Color  color{};
      float  thickness{};
    } add_quad_bezier{};

    struct
    {
      float2 p0{};
      float2 p1{};
      float2 p2{};
      float2 p3{};
      Color  color{};
      float  thickness{};
    } add_cubic_bezier{};

    struct
    {
      ImageHandle handle{};
      float2      left_top{};
      float2      right_bottom{};
      uint8_t     alpha{};
    } add_image{};

    struct
    {
      float2 p0{};
    } path_begin{};

    struct
    {
      float2 p{};
    } add_path_line_to{};

    struct
    {
      float2 center{};
      float2 p1{};
      bool   ccw{};
    } add_path_arc_to{};

    struct
    {
      float2 p1{};
      float2 p2{};
    } add_path_quad_bezier_to{};

    struct
    {
      float2 p1{};
      float2 p2{};
      float2 p3{};
    } add_path_cubic_bezier_to{};

    struct
    {
      bool   close{};
      Color  color{};
      float  thickness{};
    } path_end{};

    struct
    {
      Rect rect{};
    } add_scissor_rect{};

    struct
    {
      uint32_t count{};
    } discard_beg{};

    struct
    {
    } discard_end{};

    struct
    {
    } union_beg{};

    struct
    {
      Color color{};
      float thickness{};
    } union_end{};
  };

  void add_rect(float2 left_top, float2 right_bottom, Color color, float thickness) noexcept;
  void add_triangle(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept;
  void add_circle(float2 center, float radius, Color color, float thickness) noexcept;
  void add_line(float2 p0, float2 p1, Color color, float thickness) noexcept;
  void add_arc(float2 center, float2 p0, float2 p1, Color color, float thickness) noexcept;
  void add_quad_bezier(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept;
  void add_cubic_bezier(float2 p0, float2 p1, float2 p2, float2 p3, Color color, float thickness) noexcept;
  void add_image(ImageHandle handle, float2 left_top, float2 right_bottom, uint8_t alpha) noexcept;

  void path_begin(float2 p0) noexcept;
  void add_path_line_to(float2 p) noexcept;
  void add_path_arc_to(float2 center, float2 p1, bool ccw) noexcept;
  void add_path_quad_bezier_to(float2 p1, float2 p2) noexcept;
  void add_path_cubic_bezier_to(float2 p1, float2 p2, float2 p3) noexcept;
  void path_end(bool close, Color color, float thickness) noexcept;

  void add_scissor_rect(Rect rect) noexcept;

  void discard_beg(std::function<void()> func) noexcept;
  void discard_end() noexcept;
  void union_beg() noexcept;
  void union_end(Color color = {}, float thickness = {}) noexcept;

private:
  void _add_rect(float2 left_top, float2 right_bottom, Color color, float thickness) noexcept;
  void _add_triangle(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept;
  void _add_circle(float2 center, float radius, Color color, float thickness) noexcept;
  void _add_line(float2 p0, float2 p1, Color color, float thickness) noexcept;
  void _add_arc(float2 center, float2 p0, float2 p1, Color color, float thickness) noexcept;
  void _add_quad_bezier(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept;
  void _add_cubic_bezier(float2 p0, float2 p1, float2 p2, float2 p3, Color color, float thickness) noexcept;
  void _add_image(ImageHandle handle, float2 left_top, float2 right_bottom, uint8_t alpha) noexcept;

  void _path_begin(float2 p0) noexcept;
  void _add_path_line_to(float2 p) noexcept { _points.emplace_back(p); }
  void _add_path_arc_to(float2 center, float2 p1, bool ccw) noexcept;
  void _add_path_quad_bezier_to(float2 p1, float2 p2) noexcept;
  void _add_path_cubic_bezier_to(float2 p1, float2 p2, float2 p3) noexcept;
  void _path_end(bool close, Color color, float thickness) noexcept;

  void _add_scissor_rect(Rect rect) noexcept;

  void _discard_beg(uint count, uint& idx) noexcept;
  void _discard_end() noexcept;
  void _union_beg() noexcept;
  void _union_end(Color color, float thickness) noexcept;

public:
  void build_render_cmd(DrawCmd const& cmd, uint& idx) noexcept;
  void build_render_cmds() noexcept;

  void set_window_pos(float2 pos) noexcept { _window_pos = pos; }
  void set_window_shadow(Rect scissor_rect, uint2 window_extent, float shadow_thickness, Color color, float radius, float softness, std::optional<float4> wireframe_color) noexcept
  {
    _window_shadow_info = { scissor_rect, window_extent, shadow_thickness, { color.r, color.g, color.b }, radius, softness, wireframe_color };
  }

  auto& window_shadow_info() const noexcept { return _window_shadow_info; }
  auto& vertices()           const noexcept { return _vertices;           }
  auto& indices()            const noexcept { return _indices;            }
  auto& render_cmds()        const noexcept { return _render_cmds;        }

  auto window_pos() const noexcept { return _window_pos;                   }
  auto check()      const noexcept { return _render_cmd_rect_idxs.empty(); }

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
  auto get_rect(uint32_t vtx_beg, uint32_t vtx_cnt) const noexcept -> Rect;
  void add_rect(float2 left_top, float2 right_bottom, Color color = {}) noexcept;

  void push_render_cmd(RenderCmdType type, ImageHandle image_handle = Write_Image_Handle) noexcept;
  void push_render_cmd_clear_rect(RenderCmdType type, std::optional<Rect> rect = {}) noexcept;

  void add_convex_poly_filled(Color color) noexcept;
  void add_concave_poly_filled(Color color) noexcept;

  static auto calc_circle_segment_count(float radius) noexcept -> float;
  static auto get_circle_segment_count(float radius) noexcept -> uint32_t;

  void add_poly_line(Color color, float thickness, bool is_closed) noexcept;
  void path_arc_to(float2 center, float radius, float min, float max) noexcept;
  void _path_arc_to(float2 center, float radius, int min, int max) noexcept;
  void _path_arc_to(float2 center, float radius, int min, int max, int segment_cnt) noexcept;

  void path_bezier_quad_curve_to_casteljau(float2 p0, float2 p1, float2 p2, float tess_tol, int level) noexcept;
  void path_bezier_cubic_curve_to_casteljau(float2 p0, float2 p1, float2 p2, float2 p3, float tess_tol, int level) noexcept;

private:
  std::vector<DrawCmd>  _draw_cmds;
  std::vector<Vertex>   _vertices;
  std::vector<uint16_t> _indices;
  uint32_t              _vertex_beg{};
  uint32_t              _index_beg{};

  uint32_t _tmp_vertices_size{};
  uint32_t _tmp_indices_size{};

  std::vector<RenderCmd> _render_cmds;
  std::vector<uint32_t>  _render_cmd_rect_idxs;
  uint32_t               _draw_index_beg{};

  float2 _window_pos{};

  std::optional<WindowShadowInfo> _window_shadow_info;

  std::vector<float2> _points;
  std::vector<float2> _normals;
  std::vector<float2> _tmp_buf;

  bool     _using_discard_shapes{};
  bool     _use_discard{};
  uint32_t _discard_vtx_beg{};
  uint     _clear_composite_image_cmd_idx{};

  inline static auto constexpr arc_table_size         = 48;
  inline static auto constexpr arc_sample_max         = arc_table_size;
  inline static auto constexpr curve_tessellation_tol = 1.25f;
  inline static auto constexpr tessellation_max_error = 0.3f;
  inline static auto           arc_radius_cutoff      = 0.f;
  inline static std::array<int, arc_table_size + 16> _circle_segment_counts;
  inline static std::array<float2, arc_table_size>   _arc_vertices;
};

}
