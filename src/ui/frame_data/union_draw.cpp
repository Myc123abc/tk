#include "frame_data.hpp"
#include "util.hpp"

#include <clipper2/clipper.h>

namespace tk::ui {

void FrameData::union_beg() noexcept
{
  _draw_cmds.emplace_back(DrawCmd::Type::union_beg);
}

void FrameData::union_end(Color color, float thickness) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::union_end);
  cmd.data.union_end = { color, thickness };
}

void FrameData::_union_beg(uint& idx) noexcept
{
  using namespace Clipper2Lib;

  assert(!_build_mode.contains(BuildMode::uni));
  _build_mode.add(BuildMode::uni);

  auto subjects = PathsD{};

  auto add_current_path = [&]
  {
    if (_points.size() > 1 && _points.front() == _points.back())
      _points.pop_back();

    if (_points.size() < 3)
    {
      _points.clear();
      return;
    }

    auto& path = subjects.emplace_back();
    path.reserve(_points.size());
    for (auto const p : _points)
      path.emplace_back(static_cast<double>(p.x), static_cast<double>(p.y));

    _points.clear();
  };

  auto union_color     = Color{};
  auto union_thickness = 0.f;
  auto found_end       = false;

  while (++idx < _draw_cmds.size())
  {
    auto const& cmd = _draw_cmds[idx];
    switch (cmd.type)
    {
    case DrawCmd::Type::add_rect:
      assert(_points.empty());
      _points.emplace_back(cmd.data.add_rect.left_top);
      _points.emplace_back(cmd.data.add_rect.right_bottom.x, cmd.data.add_rect.left_top.y);
      _points.emplace_back(cmd.data.add_rect.right_bottom);
      _points.emplace_back(cmd.data.add_rect.left_top.x, cmd.data.add_rect.right_bottom.y);
      add_current_path();
      break;

    case DrawCmd::Type::add_triangle:
      assert(_points.empty());
      _points.emplace_back(cmd.data.add_triangle.p0);
      _points.emplace_back(cmd.data.add_triangle.p1);
      _points.emplace_back(cmd.data.add_triangle.p2);
      add_current_path();
      break;

    case DrawCmd::Type::add_circle:
      assert(_points.empty());
      path_arc_to(cmd.data.add_circle.center, cmd.data.add_circle.radius - .5f, 0, std::numbers::pi_v<float> * 2.f);
      if (!_points.empty())
        _points.pop_back();
      add_current_path();
      break;

    case DrawCmd::Type::path_begin:
      _points.clear();
      _path_begin(cmd.data.path_begin.p0);
      break;

    case DrawCmd::Type::add_path_line_to:
      if (!_points.empty())
        _add_path_line_to(cmd.data.add_path_line_to.p);
      break;

    case DrawCmd::Type::add_path_arc_to:
      if (!_points.empty())
        _add_path_arc_to(cmd.data.add_path_arc_to.center, cmd.data.add_path_arc_to.p1, cmd.data.add_path_arc_to.ccw);
      break;

    case DrawCmd::Type::add_path_quad_bezier_to:
      if (!_points.empty())
        _add_path_quad_bezier_to(cmd.data.add_path_quad_bezier_to.p1, cmd.data.add_path_quad_bezier_to.p2);
      break;

    case DrawCmd::Type::add_path_cubic_bezier_to:
      if (!_points.empty())
        _add_path_cubic_bezier_to(cmd.data.add_path_cubic_bezier_to.p1, cmd.data.add_path_cubic_bezier_to.p2, cmd.data.add_path_cubic_bezier_to.p3);
      break;

    case DrawCmd::Type::path_end:
      if (cmd.data.path_end.close)
        add_current_path();
      else
        _points.clear();
      break;

    case DrawCmd::Type::union_end:
      union_color     = cmd.data.union_end.color;
      union_thickness = cmd.data.union_end.thickness;
      found_end       = true;
      break;

    case DrawCmd::Type::discard_beg:
    case DrawCmd::Type::discard_end:
      assert(false && "discard operations cannot be nested or crossed");
      std::unreachable();

    default:
      std::unreachable();
      break;
    }

    if (found_end)
      break;
  }

  _build_mode.remove(BuildMode::uni);
  _points.clear();
  if (!found_end || subjects.empty())
    return;

  if (_using_discard_shapes) union_color.a = 1.f;
  else if (union_color.a == 0) return;

  auto solution = Union(subjects, FillRule::NonZero, 2);
  for (auto const& path : solution)
  {
    if (path.size() < 3)
      continue;

    _points.reserve(path.size());
    for (auto const& pt : path)
      _points.emplace_back(static_cast<float>(pt.x), static_cast<float>(pt.y));

    if (union_thickness > 0)
      add_poly_line(union_color, union_thickness, true);
    else if (is_convex(_points))
      add_convex_poly_filled(union_color);
    else
      add_concave_poly_filled(union_color);
  }
}

}
