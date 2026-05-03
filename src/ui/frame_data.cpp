#include "frame_data.hpp"

using namespace tk;

namespace {

constexpr float Pi = 3.14159265358979323846f;
constexpr float Tau = Pi * 2.0f;

auto normalize_angle(float a) noexcept -> float
{
  a = std::fmod(a, Tau);
  if (a < 0.0f)
    a += Tau;
  return a;
}

auto angle_in_arc(float a, float start, float end, bool ccw) noexcept -> bool
{
  a     = normalize_angle(a);
  start = normalize_angle(start);
  end   = normalize_angle(end);

  if (ccw)
  {
    if (start <= end)
      return a >= start && a <= end;
    return a >= start || a <= end;
  }
  else
  {
    if (end <= start)
      return a <= start && a >= end;
    return a <= start || a >= end;
  }
}

auto rect_min_dist_sq(Rect r, float2 p) noexcept -> float
{
  float dx = 0.0f;
  if (p.x < r.left)
    dx = r.left - p.x;
  else if (p.x > r.right)
    dx = p.x - r.right;

  float dy = 0.0f;
  if (p.y < r.top)
    dy = r.top - p.y;
  else if (p.y > r.bottom)
    dy = p.y - r.bottom;

  return dx * dx + dy * dy;
}

auto rect_max_dist_sq(Rect r, float2 p) noexcept -> float
{
  float dx = std::max(std::abs(p.x - r.left), std::abs(p.x - r.right));
  float dy = std::max(std::abs(p.y - r.top),  std::abs(p.y - r.bottom));

  return dx * dx + dy * dy;
}

auto rect_angle_hit(Rect r, float2 center, float start, float end, bool ccw) noexcept -> bool
{
  float2 corners[4] =
  {
    { r.left,  r.top    },
    { r.right, r.top    },
    { r.right, r.bottom },
    { r.left,  r.bottom }
  };

  for (auto p : corners)
  {
    float a = std::atan2(p.y - center.y, p.x - center.x);
    if (angle_in_arc(a, start, end, ccw))
      return true;
  }

  // Also test if arc start/end ray points are inside tile directionally.
  // This avoids missing tiles when the arc passes through a tile but no corner angle is inside.
  return false;
}

auto bezier_quad_point(float2 p0, float2 p1, float2 p2, float t) noexcept -> float2
{
  auto u = 1.f - t;
  return p0 * (u * u) + p1 * (2.f * u * t) + p2 * (t * t);
}

auto bezier_cubic_point(float2 p0, float2 p1, float2 p2, float2 p3, float t) noexcept -> float2
{
  auto u  = 1.f - t;
  auto uu = u * u;
  auto tt = t * t;

  return
    p0 * (uu * u) +
    p1 * (3.f * uu * t) +
    p2 * (3.f * u * tt) +
    p3 * (tt * t);
}

}

namespace tk::ui {

void FrameData::clear() noexcept
{
  _tile_size  = {};
  _tile_count = {};
  _tiles.clear();
}

void FrameData::init(uint width, uint height, uint2 tile_size) noexcept
{
  clear();

  _tile_size  = tile_size;
  _tile_count = (float2{ width, height } + tile_size - 1) / tile_size;
  _tiles.resize(_tile_count.x + _tile_count.y);
}

////////////////////////////////////////////////////////////////////////////////
///                            Add Commands
////////////////////////////////////////////////////////////////////////////////

void FrameData::add_command(uint cmd_idx, float2 p0, float2 p1, float thickness) noexcept
{
  assert(thickness > 0.f);

  auto w = std::abs(p1.x - p0.x);
  auto h = std::abs(p1.y - p0.y);

  if (w <= _tile_size.x * 2.f && h <= _tile_size.y * 2.f)
  {
    auto r = thickness * .5f + 2.f;
    add_command(cmd_idx, { std::min(p0.x, p1.x) - r, std::min(p0.y, p1.y) - r, std::max(p0.x, p1.x) + r, std::max(p0.y, p1.y) + r });
    return;
  }

  // convert to tile space
  float inv_w = 1.0f / _tile_size.x;
  float inv_h = 1.0f / _tile_size.y;

  float tx0f = p0.x * inv_w;
  float ty0f = p0.y * inv_h;
  float tx1f = p1.x * inv_w;
  float ty1f = p1.y * inv_h;

  int x0 = (int)std::floor(tx0f);
  int y0 = (int)std::floor(ty0f);
  int x1 = (int)std::floor(tx1f);
  int y1 = (int)std::floor(ty1f);

  int dx = std::abs(x1 - x0);
  int dy = std::abs(y1 - y0);

  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;

  int err = dx - dy;

  // thickness in tile units
  float half_t = thickness * 0.5f;
  int rx = (int)std::ceil(half_t * inv_w) + 1;
  int ry = (int)std::ceil(half_t * inv_h) + 1;

  int x = x0;
  int y = y0;

  while (true)
  {
    // expand for thickness (fast square expansion)
    for (int oy = -ry; oy <= ry; ++oy)
    {
      int ty = y + oy;
      if (ty < 0 || ty >= (int)_tile_count.y)
        continue;

      for (int ox = -rx; ox <= rx; ++ox)
      {
        int tx = x + ox;
        if (tx < 0 || tx >= (int)_tile_count.x)
          continue;

        uint32_t idx = ty * _tile_count.x + tx;
        Tile& tile = _tiles[idx];

        // dedup per frame
        auto& idxs = tile.cmd_idxs;
        if (idxs.empty() || idxs.back() != cmd_idx)
          tile.cmd_idxs.push_back(cmd_idx);
      }
    }

    if (x == x1 && y == y1)
      break;

    int e2 = err << 1;

    if (e2 > -dy)
    {
      err -= dy;
      x += sx;
    }

    if (e2 < dx)
    {
      err += dx;
      y += sy;
    }
  }
}

void FrameData::add_command(uint cmd_idx, Rect rect) noexcept
{
  assert(rect.left <= rect.right && rect.top <= rect.bottom);

  auto constexpr eps = 0.001f;

  auto min_x = static_cast<int>(std::floor(rect.left / _tile_size.x));
  auto min_y = static_cast<int>(std::floor(rect.top  / _tile_size.y));
  auto max_x = static_cast<int>(std::floor((rect.right - eps) / _tile_size.x));
  auto max_y = static_cast<int>(std::floor((rect.bottom - eps)  / _tile_size.y));

  min_x = std::clamp(min_x, 0, static_cast<int>(_tile_count.x - 1));
  min_y = std::clamp(min_y, 0, static_cast<int>(_tile_count.y - 1));
  max_x = std::clamp(max_x, 0, static_cast<int>(_tile_count.x - 1));
  max_y = std::clamp(max_y, 0, static_cast<int>(_tile_count.y - 1));

  for (auto y = min_y; y <= max_y; ++y)
    for (auto x = min_x; x <= max_x; ++x)
    {
      auto& idxs = _tiles[y * _tile_count.x + x].cmd_idxs;
      if (idxs.empty() || idxs.back() != cmd_idx)
        idxs.emplace_back(cmd_idx);
    }
}

void FrameData::add_command(uint cmd_idx, float2 center, float radius, float start_angle, float end_angle, bool ccw, float thickness) noexcept
{
  assert(thickness > 0);
  float sweep = end_angle - start_angle;

  if (ccw && sweep < 0.0f)
    sweep += Tau;
  if (!ccw && sweep > 0.0f)
    sweep -= Tau;

  int segments = std::max(1, (int)std::ceil(std::abs(sweep) * radius / 16.0f));

  float2 prev
  {
    center.x + std::cos(start_angle) * radius,
    center.y + std::sin(start_angle) * radius
  };

  for (int i = 1; i <= segments; ++i)
  {
    float t = (float)i / (float)segments;
    float a = start_angle + sweep * t;

    float2 p
    {
      center.x + std::cos(a) * radius,
      center.y + std::sin(a) * radius
    };

    add_command(cmd_idx, prev, p, thickness);
    prev = p;
  }
}

////////////////////////////////////////////////////////////////////////////////
///                             Commands
////////////////////////////////////////////////////////////////////////////////

void FrameData::add_rect(float2 left_top, float2 right_bottom, Color color, float thickness) noexcept
{
  if (left_top.x >= right_bottom.x || left_top.y >= right_bottom.y)
    return;

  auto cmd_idx = _cmds.size();

  auto& cmd = _cmds.emplace_back(Command{});
  cmd.type      = Command::Type::add_rect;
  cmd.color     = color;
  cmd.thickness = thickness;
  cmd.rect      = { left_top, right_bottom };

  if (thickness > 0.f)
  {
    add_command(cmd_idx, left_top, { right_bottom.x, left_top.y }, thickness);
    add_command(cmd_idx, { right_bottom.x, left_top.y }, right_bottom, thickness);
    add_command(cmd_idx, right_bottom, { left_top.x, right_bottom.y }, thickness);
    add_command(cmd_idx, { left_top.x, right_bottom.y }, left_top, thickness);
  }
  else
    add_command(cmd_idx, { left_top, right_bottom });
}

void FrameData::add_triangle(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept
{
  // reject degenerate triangle
  auto area2 = (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x);
  if (std::abs(area2) <= 1e-6f)
      return;

  auto cmd_idx = _cmds.size();

  auto& cmd = _cmds.emplace_back(Command{});
  cmd.type      = Command::Type::add_triangle;
  cmd.color     = color;
  cmd.thickness = thickness;
  cmd.triangle  = { p0, p1, p2 };

  if (thickness > 0.f)
  {
    add_command(cmd_idx, p0, p1, thickness);
    add_command(cmd_idx, p1, p2, thickness);
    add_command(cmd_idx, p2, p0, thickness);
  }
  else
  {
    auto left   = std::min({ p0.x, p1.x, p2.x });
    auto right  = std::max({ p0.x, p1.x, p2.x });
    auto top    = std::min({ p0.y, p1.y, p2.y });
    auto bottom = std::max({ p0.y, p1.y, p2.y });

    add_command(cmd_idx, { left, top, right, bottom });
  }
}

void FrameData::add_circle(float2 center, float radius, Color color, float thickness) noexcept
{
  if (radius <= 0.f) return;

  radius = std::max(radius - 1.f, 1.f);

  auto cmd_idx = _cmds.size();

  auto& cmd = _cmds.emplace_back(Command{});
  cmd.type      = Command::Type::add_circle;
  cmd.color     = color;
  cmd.thickness = thickness;
  cmd.circle    = { center, radius };

  if (thickness > 0.f)
  {
    if (radius < _tile_size.x * 2.f || thickness >= radius * 0.5f)
    {
      float r = radius + thickness * .5f + 2.f;
      add_command(cmd_idx, Rect{ center.x - r, center.y - r, center.x + r, center.y + r });
    }
    else
      add_command(cmd_idx, center, radius, 0, Tau, true, thickness);
  }
  else
  {
    float r = radius + 2.f;
    add_command(cmd_idx, Rect{ center.x - r, center.y - r, center.x + r, center.y + r });
  }
}

void FrameData::add_line(float2 p0, float2 p1, Color color, float thickness) noexcept
{
  auto cmd_idx = _cmds.size();

  auto& cmd = _cmds.emplace_back(Command{});
  cmd.type      = Command::Type::add_line;
  cmd.color     = color;
  cmd.thickness = thickness;
  cmd.line      = { p0, p1 };

  add_command(cmd_idx, p0, p1, thickness);
}

void FrameData::add_bezier_quad(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept
{
  if (p0 == p1 && p1 == p2) return;

  auto cmd_idx = _cmds.size();

  auto& cmd = _cmds.emplace_back(Command{});
  cmd.type        = Command::Type::add_bezier_quad;
  cmd.color       = color;
  cmd.thickness   = thickness;
  cmd.bezier_quad = { p0, p1, p2 };

  auto chord = length(p2 - p0);
  auto ctrl  = length(p1 - p0) + length(p2 - p1);

  auto curve_len = std::max(chord, ctrl);

  auto segments = std::clamp(static_cast<int>(std::ceil(curve_len / 16.f)), 4, 64);

  if (thickness > 0.f)
  {
    auto prev = p0;

    for (int i = 1; i <= segments; ++i)
    {
      auto t = static_cast<float>(i) / static_cast<float>(segments);
      auto p = bezier_quad_point(p0, p1, p2, t);

      add_command(cmd_idx, prev, p, thickness);
      prev = p;
    }
  }
  else
  {
    auto left   = std::min({ p0.x, p1.x, p2.x });
    auto right  = std::max({ p0.x, p1.x, p2.x });
    auto top    = std::min({ p0.y, p1.y, p2.y });
    auto bottom = std::max({ p0.y, p1.y, p2.y });

    add_command(cmd_idx, Rect{ left, top, right, bottom });
  }
}

void FrameData::add_bezier_cubic(float2 p0, float2 p1, float2 p2, float2 p3, Color color, float thickness) noexcept
{
  if (p0 == p1 && p1 == p2 && p2 == p3)
    return;

  auto cmd_idx = _cmds.size();

  auto& cmd = _cmds.emplace_back(Command{});
  cmd.type         = Command::Type::add_bezier_cubic;
  cmd.color        = color;
  cmd.thickness    = thickness;
  cmd.bezier_cubic = { p0, p1, p2, p3 };

  auto ctrl_len  = length(p1 - p0) + length(p2 - p1) + length(p3 - p2);
  auto chord_len = length(p3 - p0);
  auto curve_len = std::max(ctrl_len, chord_len);

  auto segments = std::clamp(static_cast<int>(std::ceil(curve_len / 16.f)), 4, 96);

  if (thickness > 0.f)
  {
    auto prev = p0;

    for (auto i = 1; i <= segments; ++i)
    {
      auto t = static_cast<float>(i) / static_cast<float>(segments);
      auto p = bezier_cubic_point(p0, p1, p2, p3, t);

      add_command(cmd_idx, prev, p, thickness);
      prev = p;
    }
  }
  else
  {
    auto left   = std::min({ p0.x, p1.x, p2.x, p3.x });
    auto right  = std::max({ p0.x, p1.x, p2.x, p3.x });
    auto top    = std::min({ p0.y, p1.y, p2.y, p3.y });
    auto bottom = std::max({ p0.y, p1.y, p2.y, p3.y });

    add_command(cmd_idx, Rect{ left, top, right, bottom });
  }
}

void FrameData::add_image(ImageHandle handle, float2 left_top, float2 right_bottom, uint8_t alpha) noexcept
{
  if (alpha == 0 || left_top.x >= right_bottom.x || left_top.y >= right_bottom.y)
    return;

  auto cmd_idx = _cmds.size();

  auto& cmd = _cmds.emplace_back(Command{});
  cmd.type  = Command::Type::add_rect;
  cmd.color = { 1, 1, 1, static_cast<float>(alpha) / 255};
  cmd.image = { handle, left_top, right_bottom };

  add_command(cmd_idx, { left_top, right_bottom });
}

}
