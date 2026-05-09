#pragma once

#include "ui/ui.hpp"
#include "util/rect.hpp"
#include "image_manager.hpp"

namespace tk::ui {

enum class RenderCallType
{
  ui,
  window_shadow,
};

struct RenderCall
{
  RenderCallType type;
  Rect           scissor_rect;
  uint2          window_pos;

  union
  {
    struct
    {
      uint2                 window_extent{};
      float                 shadow_thickness{};
      float3                color{};
      float                 radius{};
      float                 softness{};
      std::optional<float4> wireframe_color;
    } window_shadow;
  };
};

enum class DrawCmdType
{
  add_rect,
  add_triangle,
  add_circle,
  add_line,
  add_arc,
  add_quad_bezier,
  add_image,

  add_path,
  add_path_line,
  add_path_arc,
  add_path_quad_bezier,
};

enum class DrawCmdOp
{
  none,
  uni,
};

struct DrawCmd
{
  DrawCmdType type{};
  Color       color;
  float       thickness{};
  DrawCmdOp   op{};
  uint        uni_cnt{};

  union
  {
    struct
    {
      float2 left_top;
      float2 right_bottom;
    } rect;

    struct
    {
      float2 p0;
      float2 p1;
      float2 p2;
    } triangle;

    struct
    {
      float2 center;
      float radius;
    } circle;

    struct
    {
      float2 p0;
      float2 p1;
      uint   no_aa; // hlsl make bool as uint
    } line;

    struct
    {
      float2 center;
      float2 p0;
      float2 p1;
    } arc;

    struct
    {
      float2 p0;
      float2 p1;
      float2 p2;
    } quad_bezier;

    struct
    {
      float2 left_top;
      float2 right_bottom;
      int    idx; // use int because descriptor handle's index is int, -1 for validation
    } image;

    struct
    {
      uint beg;
      uint count;
    } path;

    struct
    {
      float2 p0;
      float2 p1;
    } path_line;

    struct
    {
      float2 p0;
      float2 p1;
      float2 p2;
    } path_quad_bezier;

    struct
    {
      float2 center;
      float2 p0;
      float2 p1;
    } path_arc;
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

  void init(uint width, uint height, uint2 tile_size = { 128, 128 }) noexcept;
  void clear() noexcept;

  //
  // Commands
  //
  void add_rect(float2 left_top, float2 right_bottom, Color color, float thickness) noexcept;
  void add_triangle(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept;
  void add_circle(float2 center, float radius, Color color, float thickness) noexcept;
  void add_line(float2 p0, float2 p1, Color color, float thickness) noexcept;
  void add_arc(float2 center, float2 p0, float2 p1, Color color, float thickness) noexcept;
  void add_quad_bezier(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept;
  void add_cubic_bezier(float2 p0, float2 p1, float2 p2, float2 p3, Color color, float thickness) noexcept;
  void add_image(ImageHandle handle, float2 left_top, float2 right_bottom, uint8_t alpha) noexcept;

  void path_begin(float2 p0) noexcept;
  void add_path_line_to(float2 p1) noexcept;
  void add_path_arc_to(float2 center, float2 p1, bool ccw) noexcept;
  void add_path_quad_bezier_to(float2 p1, float2 p2) noexcept;
  void add_path_cubic_bezier_to(float2 p1, float2 p2, float2 p3) noexcept;
  void path_end(bool close, Color color, float thickness) noexcept;

  void union_beg() noexcept;
  void union_end(Color color, float thickness) noexcept;

  void build_ui_render_call(Rect rect, uint2 window_pos) noexcept;
  void build_window_shadow_render_call(Rect scissor_rect, uint2 window_pos, uint2 window_extent, float shadow_thickness, Color color, float radius, float softness, std::optional<float4> wireframe_color) noexcept;

  auto window_extent() const noexcept { return _window_extent; }
  auto tile_size()     const noexcept { return _tile_size;     }
  auto tile_count()    const noexcept { return _tile_count;    }

  auto& render_calls() const noexcept { return _calls;     }
  auto& cmds()         const noexcept { return _cmds;      }
  auto& path_cmds()    const noexcept { return _path_cmds; }
  auto& cmd_idxs()     const noexcept { return _cmd_idxs;  }
  auto& gpu_tiles()    const noexcept { return _gpu_tiles; }

private:
  void add_command(uint cmd_idx, Rect rect) noexcept;
  void add_command(uint cmd_idx, float2 p0, float2 p1, float thickness) noexcept;
  void add_command(uint cmd_idx, float2 center, float2 p0, float2 p1, float thickness) noexcept;
  void add_command(uint cmd_idx, float2 center, float radius, float start_angle, float end_angle, bool ccw, float thickness) noexcept;

  void cubic_bezier_to_quad(float2 p0, float2 p1, float2 p2, float2 p3, float tolerance = 0.5, uint level = 0) noexcept;

private:
  struct Tile
  {
    std::vector<uint> cmd_idxs;
  };

  uint2                   _tile_size;
  uint2                   _tile_count;
  uint2                   _window_extent;
  std::vector<Tile>       _tiles;
  std::vector<DrawCmd>    _cmds;
  std::vector<DrawCmd>    _path_cmds;
  std::vector<uint>       _cmd_idxs;

  struct GPUTile
  {
    uint beg;
    uint count;
  };
  std::vector<GPUTile>    _gpu_tiles;

  std::vector<RenderCall> _calls;
  DrawCmd*                _path_cmd{};
  float2                  _path_point{};
  float2                  _path_beg_point{};

  std::vector<std::tuple<float2, float2, float2>> _quad_beziers;

  DrawCmdOp _op{};
  uint      _uni_cmd_beg{};
};

}
