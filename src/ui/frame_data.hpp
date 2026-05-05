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
  add_bezier_quad,
  add_bezier_cubic,
  add_image,
};

struct DrawCmd
{
  DrawCmdType type;
  Color       color;
  float       thickness{};

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
      float2 p0;
      float2 p1;
      float2 p2;
    } bezier_quad;

    struct
    {
      float2 p0;
      float2 p1;
      float2 p2;
      float2 p3;
    } bezier_cubic;

    struct
    {
      float2 left_top;
      float2 right_bottom;
      int    idx; // use int because descriptor handle's index is int, -1 for validation
    } image;
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
  void add_bezier_quad(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept;
  void add_bezier_cubic(float2 p0, float2 p1, float2 p2, float2 p3, Color color, float thickness) noexcept;
  void add_image(ImageHandle handle, float2 left_top, float2 right_bottom, uint8_t alpha) noexcept;

  void build_ui_render_call(Rect rect, uint2 window_pos) noexcept;
  void build_window_shadow_render_call(Rect scissor_rect, uint2 window_pos, uint2 window_extent, float shadow_thickness, Color color, float radius, float softness, std::optional<float4> wireframe_color) noexcept;

  auto window_extent() const noexcept { return _window_extent; }
  auto tile_size() const noexcept { return _tile_size; }
  auto tile_count() const noexcept { return _tile_count; }

  auto& render_calls() const noexcept { return _calls; }
  auto& cmds() const noexcept { return _cmds; }
  auto& cmd_idxs() const noexcept { return _cmd_idxs; }
  auto& gpu_tiles() const noexcept { return _gpu_tiles; }

private:
  void add_command(uint cmd_idx, Rect rect) noexcept;
  void add_command(uint cmd_idx, float2 p0, float2 p1, float thickness) noexcept;
  void add_command(uint cmd_idx, float2 center, float radius, float start_angle, float end_angle, bool ccw, float thickness) noexcept;

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
  std::vector<uint>       _cmd_idxs;

  struct GPUTile
  {
    uint beg;
    uint count;
  };
  std::vector<GPUTile>    _gpu_tiles;

  std::vector<RenderCall> _calls;
};

}
