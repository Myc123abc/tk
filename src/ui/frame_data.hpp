#include "ui/ui.hpp"
#include "image_manager.hpp"

namespace tk {

using uint = uint32_t;

struct Rect
{
  union
  {
    struct
    {
      float left{};
      float top{};
      float right{};
      float bottom{};
    };
    vec4 data;
  };

  Rect() = default;
  Rect(vec2 left_top, vec2 right_bottom) noexcept : data(left_top, right_bottom) {}
  Rect(float left, float top, float right, float bottom) noexcept : left(left), top(top), right(right), bottom(bottom) {}
};

}

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

  void init(uint width, uint height, vec2u tile_size = { 128, 128 }) noexcept;
  
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
        vec2 left_top;
        vec2 right_bottom;
      } rect;

      struct
      {
        vec2 p0;
        vec2 p1;
        vec2 p2;
      } triangle;

      struct
      {
        vec2  center;
        float radius;
      } circle;

      struct
      {
        vec2 p0;
        vec2 p1;
      } line;

      struct
      {
        vec2 p0;
        vec2 p1;
        vec2 p2;
      } bezier_quad;

      struct
      {
        vec2 p0;
        vec2 p1;
        vec2 p2;
        vec2 p3;
      } bezier_cubic;

      struct
      {
        ImageHandle handle;
        vec2        left_top;
        vec2        right_bottom;
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
  void add_rect(vec2 left_top, vec2 right_bottom, Color color, float thickness) noexcept;
  void add_triangle(vec2 p0, vec2 p1, vec2 p2, Color color, float thickness) noexcept;
  void add_circle(vec2 center, float radius, Color color, float thickness) noexcept;
  void add_line(vec2 p0, vec2 p1, Color color, float thickness) noexcept;
  void add_bezier_quad(vec2 p0, vec2 p1, vec2 p2, Color color, float thickness) noexcept;
  void add_bezier_cubic(vec2 p0, vec2 p1, vec2 p2, vec2 p3, Color color, float thickness) noexcept;
  void add_image(ImageHandle handle, vec2 left_top, vec2 right_bottom, uint8_t alpha) noexcept;

private:
  void clear() noexcept;
  
  void add_command(uint cmd_idx, Rect rect) noexcept;
  void add_command(uint cmd_idx, vec2 p0, vec2 p1, float thickness) noexcept;
  void add_command(uint cmd_idx, vec2 center, float radius, float start_angle, float end_angle, bool ccw, float thickness) noexcept;

private:
  vec2u                _tile_size;
  vec2u                _tile_count;
  std::vector<Tile>    _tiles;
  std::vector<Command> _cmds;
};

}
