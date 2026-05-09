#include "frame_data.hpp"
#include "../renderer/renderer/renderer.hpp"

#include <ranges>

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

auto arc_radius(float2 center, float2 p) noexcept -> float
{
  return length(p - center);
}

auto arc_angle(float2 center, float2 p) noexcept -> float
{
  return std::atan2(p.y - center.y, p.x - center.x);
}

auto arc_bounds(float2 center, float2 p0, float2 p1) noexcept -> Rect
{
  auto left   = std::min(p0.x, p1.x);
  auto right  = std::max(p0.x, p1.x);
  auto top    = std::min(p0.y, p1.y);
  auto bottom = std::max(p0.y, p1.y);

  auto radius = arc_radius(center, p0);
  auto start  = arc_angle(center, p0);
  auto end    = arc_angle(center, p1);

  for (float angle : { 0.0f, Pi * 0.5f, Pi, Pi * 1.5f })
  {
    if (!angle_in_arc(angle, start, end, true))
      continue;

    auto point = float2{ center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius };
    left   = std::min(left, point.x);
    right  = std::max(right, point.x);
    top    = std::min(top, point.y);
    bottom = std::max(bottom, point.y);
  }

  return { left, top, right, bottom };
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

auto quad_bezier_point(float2 p0, float2 p1, float2 p2, float t) noexcept -> float2
{
  auto u = 1.f - t;
  return p0 * (u * u) + p1 * (2.f * u * t) + p2 * (t * t);
}

auto cubic_bezier_point(float2 p0, float2 p1, float2 p2, float2 p3, float t) noexcept -> float2
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

auto cubic_flatness(float2 p0, float2 p1, float2 p2, float2 p3) noexcept
{
  float2 d = p3 - p0;

  float d2 = std::abs(cross(p1 - p3, d));
  float d3 = std::abs(cross(p2 - p3, d));

  return std::max(d2, d3);
}

auto cubic_to_quad_control(float2 p0, float2 p1, float2 p2, float2 p3) noexcept -> float2
{
  return (p1 * 3.f - p0 + p2 * 3.f - p3) * 0.25f;
}

}

namespace tk::ui {

void FrameData::clear() noexcept
{
  _cmds.clear();
  _path_cmds.clear();
  _cmd_idxs.clear();
  for (auto& tile : _tiles)
    tile.cmd_idxs.clear();
  _gpu_tiles.clear();
  _gpu_tiles.resize(_tiles.size());
  _calls.clear();
}

void FrameData::init(uint width, uint height, uint2 tile_size) noexcept
{
  _window_extent = { width, height };
  _tile_size     = tile_size;
  _tile_count    = (float2{ width, height } + tile_size - 1) / tile_size;
  _tiles.clear();
  _tiles.resize(_tile_count.x * _tile_count.y);
  _gpu_tiles.resize(_tiles.size());
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
  assert(!rect.empty());

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

void FrameData::add_command(uint cmd_idx, float2 center, float2 p0, float2 p1, float thickness) noexcept
{
  auto radius = arc_radius(center, p0);
  if (radius <= 0.0f)
  {
    add_command(cmd_idx, Rect{ center.x, center.y, center.x, center.y });
    return;
  }

  add_command(cmd_idx, center, radius, arc_angle(center, p0), arc_angle(center, p1), true, thickness);
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
  assert(!_path_cmd);

  if (left_top.x >= right_bottom.x || left_top.y >= right_bottom.y)
    return;

  auto cmd_idx = _cmds.size();

  auto& cmd = _cmds.emplace_back(DrawCmd{});
  cmd.type      = DrawCmdType::add_rect;
  cmd.color     = color;
  cmd.thickness = thickness;
  cmd.rect      = { left_top, right_bottom };
  cmd.op        = _op;

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
  assert(!_path_cmd);

  // reject degenerate triangle
  auto area2 = (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x);
  if (std::abs(area2) <= 1e-6f)
      return;

  auto cmd_idx = _cmds.size();

  auto& cmd = _cmds.emplace_back(DrawCmd{});
  cmd.type      = DrawCmdType::add_triangle;
  cmd.color     = color;
  cmd.thickness = thickness;
  cmd.triangle  = { p0, p1, p2 };
  cmd.op        = _op;

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
  assert(!_path_cmd);

  if (radius <= 0.f) return;

  radius = std::max(radius - 1.f, 1.f);

  auto cmd_idx = _cmds.size();

  auto& cmd = _cmds.emplace_back(DrawCmd{});
  cmd.type      = DrawCmdType::add_circle;
  cmd.color     = color;
  cmd.thickness = thickness;
  cmd.circle    = { center, radius };
  cmd.op        = _op;

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
  assert(!_path_cmd);

  auto cmd_idx = _cmds.size();

  auto dp = abs(p1 - p0);

  auto& cmd = _cmds.emplace_back(DrawCmd{});
  cmd.type      = DrawCmdType::add_line;
  cmd.color     = color;
  cmd.thickness = thickness;
  cmd.line      = { p0, p1, thickness > 1 ? false : dp.x < 1e-5 || dp.y < 1e-5 };
  cmd.op        = _op;

  add_command(cmd_idx, p0, p1, thickness);
}

void FrameData::add_arc(float2 center, float2 p0, float2 p1, Color color, float thickness) noexcept
{
  assert(!_path_cmd);

  if (center == p0 && center == p1)
    return;

  auto cmd_idx = _cmds.size();

  auto& cmd = _cmds.emplace_back(DrawCmd{});
  cmd.type      = DrawCmdType::add_arc;
  cmd.color     = color;
  cmd.thickness = thickness;
  cmd.arc       = { center, p0, p1 };
  cmd.op        = _op;

  add_command(cmd_idx, center, p0, p1, thickness);
}

void FrameData::add_quad_bezier(float2 p0, float2 p1, float2 p2, Color color, float thickness) noexcept
{
  assert(!_path_cmd);

  if (p0 == p1 && p1 == p2) return;

  // if the quad is a line, use line
  if (std::abs(cross(p2 - p0, p1 - p0)) < 1e-5)
  {
    add_line(p0, p2, color, thickness);
    return;
  }

  auto cmd_idx = _cmds.size();

  auto& cmd = _cmds.emplace_back(DrawCmd{});
  cmd.type        = DrawCmdType::add_quad_bezier;
  cmd.color       = color;
  cmd.thickness   = thickness;
  cmd.quad_bezier = { p0, p1, p2 };
  cmd.op          = _op;

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
      auto p = quad_bezier_point(p0, p1, p2, t);

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

void FrameData::add_cubic_bezier(float2 p0, float2 p1, float2 p2, float2 p3, Color color, float thickness) noexcept
{
  assert(!_path_cmd && _quad_beziers.empty());

  if (p0 == p1 && p1 == p2 && p2 == p3)
    return;

  if (std::abs(cross(p3 - p0, p1 - p0)) < 1e-5 &&
      std::abs(cross(p3 - p0, p2 - p0)) < 1e-5)
  {
    add_line(p0, p3, color, thickness);
    return;
  }

  cubic_bezier_to_quad(p0, p1, p2, p3);

  for (auto [p0, p1, p2] : _quad_beziers)
    add_quad_bezier(p0, p1, p2, color, thickness);
  _quad_beziers.clear();
}

void FrameData::cubic_bezier_to_quad(float2 p0, float2 p1, float2 p2, float2 p3, float tolerance, uint level) noexcept
{
  auto chord = p3 - p0;
  auto chord_len_sq = length_sq(chord);

  if (level >= 10 || chord_len_sq <= 1e-6f)
  {
    _quad_beziers.emplace_back(p0, cubic_to_quad_control(p0, p1, p2, p3), p3);
    return;
  }

  auto flatness = cubic_flatness(p0, p1, p2, p3);
  if (flatness * flatness <= tolerance * tolerance * chord_len_sq)
  {
    _quad_beziers.emplace_back(p0, cubic_to_quad_control(p0, p1, p2, p3), p3);
    return;
  }

  auto p01 = (p0 + p1) * 0.5f;
  auto p12 = (p1 + p2) * 0.5f;
  auto p23 = (p2 + p3) * 0.5f;

  auto p012 = (p01 + p12) * 0.5f;
  auto p123 = (p12 + p23) * 0.5f;
  auto p0123 = (p012 + p123) * 0.5f;

  cubic_bezier_to_quad(p0, p01, p012, p0123, tolerance, level + 1);
  cubic_bezier_to_quad(p0123, p123, p23, p3, tolerance, level + 1);
}

////////////////////////////////////////////////////////////////////////////////
///                               path
////////////////////////////////////////////////////////////////////////////////

void FrameData::path_begin(float2 p0) noexcept
{
  assert(!_path_cmd);
  _path_cmd = &_cmds.emplace_back(DrawCmd{});
  _path_cmd->type     = DrawCmdType::add_path;
  _path_cmd->path.beg = _path_cmds.size();
  _path_cmd->op       = _op;
  _path_point         = p0;
  _path_beg_point     = p0;
}

void FrameData::add_path_line_to(float2 p1) noexcept
{
  assert(_path_cmd);
  auto& cmd = _path_cmds.emplace_back(DrawCmd{});
  cmd.type      = DrawCmdType::add_path_line;
  cmd.path_line = { _path_point, p1 };
  _path_point   = p1;
}

void FrameData::add_path_quad_bezier_to(float2 p1, float2 p2) noexcept
{
  assert(_path_cmd);
  auto p0 = _path_point;
  if (std::abs(cross(p2 - p0, p1 - p0)) < 1e-5)
  {
    add_path_line_to(p2);
    return;
  }
  auto& cmd = _path_cmds.emplace_back(DrawCmd{});
  cmd.type             = DrawCmdType::add_path_quad_bezier;
  cmd.path_quad_bezier = { p0, p1, p2 };
  _path_point = p2;
}

void FrameData::add_path_cubic_bezier_to(float2 p1, float2 p2, float2 p3) noexcept
{
  assert(_path_cmd && _quad_beziers.empty());
  auto p0 = _path_point;
  if (std::abs(cross(p3 - p0, p1 - p0)) < 1e-5 &&
      std::abs(cross(p3 - p0, p2 - p0)) < 1e-5)
  {
    add_path_line_to(p3);
    return;
  }

  cubic_bezier_to_quad(p0, p1, p2, p3);

  for (auto [p0, p1, p2] : _quad_beziers)
  {
    _path_point = p0;
    add_path_quad_bezier_to(p1, p2);
  }
  _quad_beziers.clear();
  _path_point = p3;
}

void FrameData::add_path_arc_to(float2 center, float2 p1, bool ccw) noexcept
{
  assert(_path_cmd);
  auto& cmd = _path_cmds.emplace_back(DrawCmd{});
  cmd.type     = DrawCmdType::add_path_arc;
  if (ccw)
    cmd.path_arc = { center, p1, _path_point };
  else
    cmd.path_arc = { center, _path_point, p1 };
  _path_point = p1;
}

void FrameData::path_end(bool close, Color color, float thickness) noexcept
{
  assert(_path_cmd);

  if (close) add_path_line_to(_path_beg_point);

  auto cmd_idx    = _cmds.size() - 1;
  auto path_beg   = _path_cmd->path.beg;
  auto path_count = _path_cmds.size() - path_beg;

  _path_cmd->color      = color;
  _path_cmd->thickness  = thickness;
  _path_cmd->path.count = path_count;

  // stroke tiles overlap
  if (thickness > 0)
  {
    for (auto i = 0; i < path_count; ++i)
    {
      auto const& cmd = _path_cmds[path_beg + i];
      switch (cmd.type)
      {
      default: std::unreachable();

      case DrawCmdType::add_path_line:
        add_command(cmd_idx, cmd.path_line.p0, cmd.path_line.p1, thickness);
        break;

      case DrawCmdType::add_path_arc:
        add_command(cmd_idx, cmd.path_arc.center, cmd.path_arc.p0, cmd.path_arc.p1, thickness);
        break;

      case DrawCmdType::add_path_quad_bezier:
      {
        auto p0 = cmd.path_quad_bezier.p0;
        auto p1 = cmd.path_quad_bezier.p1;
        auto p2 = cmd.path_quad_bezier.p2;

        auto chord = length(p2 - p0);
        auto ctrl  = length(p1 - p0) + length(p2 - p1);
        auto curve_len = std::max(chord, ctrl);
        auto segments = std::clamp(static_cast<int>(std::ceil(curve_len / 16.f)), 4, 64);

        auto prev = p0;
        for (int j = 1; j <= segments; ++j)
        {
          auto t = static_cast<float>(j) / static_cast<float>(segments);
          auto p = quad_bezier_point(p0, p1, p2, t);

          add_command(cmd_idx, prev, p, thickness);
          prev = p;
        }
        break;
      }
      }
    }
  }
  // fiiled tiles overlap
  else
  {
    auto pts = std::vector<float2>{};
    for (auto i = 0; i < path_count; ++i)
    {
      auto const& cmd = _path_cmds[path_beg + i];
      switch (cmd.type)
      {
      default: std::unreachable();

      case DrawCmdType::add_path_line:
        pts.emplace_back(cmd.path_line.p0);
        pts.emplace_back(cmd.path_line.p1);
        break;

      case DrawCmdType::add_path_quad_bezier:
        pts.emplace_back(cmd.path_quad_bezier.p0);
        pts.emplace_back(cmd.path_quad_bezier.p1);
        pts.emplace_back(cmd.path_quad_bezier.p2);
        break;

      case DrawCmdType::add_path_arc:
      {
        auto bounds = arc_bounds(cmd.path_arc.center, cmd.path_arc.p0, cmd.path_arc.p1);
        pts.emplace_back(float2{ bounds.left, bounds.top });
        pts.emplace_back(float2{ bounds.right, bounds.bottom });
        break;
      }
      }
    }
    add_command(cmd_idx, { pts });
  }

  _path_cmd = {};
}
////////////////////////////////////////////////////////////////////////////////
///                              union
////////////////////////////////////////////////////////////////////////////////

void FrameData::union_beg() noexcept
{
  assert(!_path_cmd);
  _op          = DrawCmdOp::uni;
  _uni_cmd_beg = _cmds.size();
}

void FrameData::union_end(Color color, float thickness) noexcept
{
  assert(!_path_cmd && !_cmds.empty() && _cmds.back().op == _op);
  for (auto i = _uni_cmd_beg; i < _cmds.size(); ++i)
  {
    auto& cmd = _cmds[i];
    if (cmd.op != _op) break;
    cmd.color     = color;
    cmd.thickness = thickness;
    cmd.uni_cnt   = _cmds.size() - i;
  }
  _op = {};
}

////////////////////////////////////////////////////////////////////////////////
///                              other
////////////////////////////////////////////////////////////////////////////////

void FrameData::add_image(ImageHandle handle, float2 left_top, float2 right_bottom, uint8_t alpha) noexcept
{
  assert(!_path_cmd);

  if (alpha == 0 || left_top.x >= right_bottom.x || left_top.y >= right_bottom.y)
    return;

  auto cmd_idx = _cmds.size();

  auto& cmd = _cmds.emplace_back(DrawCmd{});
  cmd.type  = DrawCmdType::add_image;
  cmd.color = { 1, 1, 1, static_cast<float>(alpha) / 255};
  cmd.image = { left_top, right_bottom, renderer::g_renderer.descriptor_idx(handle) };

  add_command(cmd_idx, { left_top, right_bottom });
}

void FrameData::build_ui_render_call(Rect rect, uint2 window_pos) noexcept
{
  assert(!_path_cmd);

  auto& call = _calls.emplace_back(RenderCall{});
  call.type         = RenderCallType::ui;
  call.scissor_rect = rect;
  call.window_pos   = window_pos;

  // copy cmd_idxs
  auto size = uint{};
  for (auto const& tile : _tiles) size += tile.cmd_idxs.size();
  _cmd_idxs.resize(size);
  _gpu_tiles.resize(_tiles.size());
  auto beg = 0u;
  for (auto i = 0u; i < _tiles.size(); ++i)
  {
    auto const& tile = _tiles[i];
    auto cnt = tile.cmd_idxs.size();
    if (cnt)
      memcpy(_cmd_idxs.data() + beg, tile.cmd_idxs.data(), sizeof(uint) * cnt);

    // build gpu tiles
    _gpu_tiles[i] = { beg, static_cast<uint>(cnt) };
    beg += cnt;
  }
}

void FrameData::build_window_shadow_render_call(Rect scissor_rect, uint2 window_pos, uint2 window_extent, float shadow_thickness, Color color, float radius, float softness, std::optional<float4> wireframe_color) noexcept
{
  auto& call = _calls.emplace_back(RenderCall{});
  call.type          = RenderCallType::window_shadow;
  call.scissor_rect  = scissor_rect;
  call.window_shadow = { window_extent, shadow_thickness, { color.r, color.g, color.b }, radius, softness, wireframe_color };
  call.window_pos    = window_pos;
}

}
