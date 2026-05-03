#include "ui/ui.hpp"
#include "util/rect.hpp"
#include "image_manager.hpp"

namespace tk::ui {

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
  
private:
  struct Command
  {
    enum class Type
    {
      add_rect,
      add_triangle,
      add_circle,
      add_line,
      add_bezier_quad,
      add_bezier_cubic,
      add_image,
    };

    Type  type;
    Color color;
    float thickness{};

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
        ImageHandle handle;
        float2      left_top;
        float2      right_bottom;
      } image;
    };
  };

  struct Tile
  {
    std::vector<uint> cmd_idxs;
  };

public:

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

private:
  void clear() noexcept;
  
  void add_command(uint cmd_idx, Rect rect) noexcept;
  void add_command(uint cmd_idx, float2 p0, float2 p1, float thickness) noexcept;
  void add_command(uint cmd_idx, float2 center, float radius, float start_angle, float end_angle, bool ccw, float thickness) noexcept;

private:
  uint2             _tile_size;
  uint2             _tile_count;
  std::vector<Tile>    _tiles;
  std::vector<Command> _cmds;
};

}
