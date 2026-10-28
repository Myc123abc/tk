#include "frame_data.hpp"
#include "util.hpp"
#include "../config.hpp"
#include "../ui_context.hpp"

#include <clipper2/clipper.h>

#include <algorithm>
#include <type_traits>

namespace {

template <typename T>
requires std::is_trivially_copyable_v<T>
void append_value(std::vector<std::byte>& key, T const& value)
{
  auto const bytes = std::as_bytes(std::span{ &value, 1uz });
  key.insert(key.end(), bytes.begin(), bytes.end());
}

void append_float2(std::vector<std::byte>& key, tk::float2 value)
{
  append_value(key, value.x);
  append_value(key, value.y);
}

void append_matrix(std::vector<std::byte>& key, tk::ui::Matrix const& matrix)
{
  append_value(key, matrix.m11);
  append_value(key, matrix.m12);
  append_value(key, matrix.m21);
  append_value(key, matrix.m22);
  append_value(key, matrix.tx);
  append_value(key, matrix.ty);
}

void append_color(std::vector<std::byte>& key, tk::ui::Color color)
{
  append_value(key, color.r);
  append_value(key, color.g);
  append_value(key, color.b);
  append_value(key, color.a);
}

void append_command(std::vector<std::byte>& key, tk::ui::FrameData::DrawCmd const& cmd)
{
  using Type = tk::ui::FrameData::DrawCmd::Type;
  append_value(key, cmd.type);
  switch (cmd.type)
  {
  case Type::add_rect:
    append_float2(key, cmd.data.add_rect.left_top);
    append_float2(key, cmd.data.add_rect.right_bottom);
    break;
  case Type::add_triangle:
    append_float2(key, cmd.data.add_triangle.p0);
    append_float2(key, cmd.data.add_triangle.p1);
    append_float2(key, cmd.data.add_triangle.p2);
    break;
  case Type::add_circle:
    append_float2(key, cmd.data.add_circle.center);
    append_value(key, cmd.data.add_circle.radius);
    break;
  case Type::path_begin:
    append_float2(key, cmd.data.path_begin.p0);
    break;
  case Type::add_path_line_to:
    append_float2(key, cmd.data.add_path_line_to.p);
    break;
  case Type::add_path_arc_to:
    append_float2(key, cmd.data.add_path_arc_to.center);
    append_float2(key, cmd.data.add_path_arc_to.p1);
    append_value(key, static_cast<uint8_t>(cmd.data.add_path_arc_to.ccw));
    break;
  case Type::add_path_quad_bezier_to:
    append_float2(key, cmd.data.add_path_quad_bezier_to.p1);
    append_float2(key, cmd.data.add_path_quad_bezier_to.p2);
    break;
  case Type::add_path_cubic_bezier_to:
    append_float2(key, cmd.data.add_path_cubic_bezier_to.p1);
    append_float2(key, cmd.data.add_path_cubic_bezier_to.p2);
    append_float2(key, cmd.data.add_path_cubic_bezier_to.p3);
    break;
  case Type::path_end:
    append_value(key, static_cast<uint8_t>(cmd.data.path_end.close));
    break;
  case Type::transform_beg:
    append_matrix(key, cmd.data.transform_beg.matrix);
    break;
  case Type::transform_end:
    break;
  case Type::union_end:
    append_color(key, cmd.data.union_end.color);
    append_value(key, cmd.data.union_end.thickness);
    break;
  default:
    tk::err_if(true, "unsupported draw command inside cached union");
  }
}

}

namespace tk::ui {

void FrameData::union_beg() noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::union_beg);
  cmd.data.union_beg = {};
}

void FrameData::union_beg(size_t cache_id) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::union_beg);
  cmd.data.union_beg = { cache_id, true };
}

void FrameData::union_end(Color color, float thickness) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::union_end);
  cmd.data.union_end = { color, thickness };
}

void FrameData::_union_beg(DrawCmd const& beg_cmd, uint& idx) noexcept
{
  using namespace Clipper2Lib;

  assert(!_build_mode.contains(BuildMode::uni));
  _build_mode.add(BuildMode::uni);

  auto cacheable     = beg_cmd.data.union_beg.cacheable;
  auto cache_key     = std::vector<std::byte>{};
  auto cache_end_idx = idx;
  if (cacheable)
  {
    auto constexpr cache_version = uint32_t{ 1 };
    append_value(cache_key, cache_version);
    append_matrix(cache_key, _transform_stack.empty() ? Matrix{} : _transform_stack.back());
    append_value(cache_key, _aa_scale);
    append_value(cache_key, static_cast<uint8_t>(_using_discard_shapes));

    while (++cache_end_idx < _draw_cmds.size())
    {
      auto const& cmd = _draw_cmds[cache_end_idx];
      append_command(cache_key, cmd);
      if (cmd.type == DrawCmd::Type::union_end)
        break;
    }
    assert(cache_end_idx < _draw_cmds.size() && _draw_cmds[cache_end_idx].type == DrawCmd::Type::union_end);
  }

  if (cacheable)
  {
    auto const& end_cmd = _draw_cmds[cache_end_idx];
    cacheable &= end_cmd.data.union_end.thickness == 0;
  }

  if (cacheable)
  {
    auto it = _union_cache.find(beg_cmd.data.union_beg.cache_id);
    if (it != _union_cache.end() && it->second.key == cache_key)
    {
      auto& cached = it->second;
      idx = cache_end_idx;
      _build_mode.remove(BuildMode::uni);
      cached.last_used = ++_union_cache_clock;
      if (should_cull(cached.bounds, 2.f))
        return;

      auto const vertex_base = _vertex_beg;
      auto [vertices, indices] = expand_beg(
        static_cast<uint>(cached.vertices.size()),
        static_cast<uint>(cached.indices.size())
      );
      std::ranges::copy(cached.vertices, vertices);
      for (auto i = 0uz; i < cached.indices.size(); ++i)
        indices[i] = static_cast<uint16>(vertex_base + cached.indices[i]);
      expand_end();
      return;
    }
  }

  auto subjects     = PathsD{};
  auto union_bounds = Rect{};

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
    auto const transform = _transform_stack.empty() ? Matrix{} : _transform_stack.back();
    for (auto const p : _points)
    {
      auto const transformed = transform.transform_precise(p);
      path.emplace_back(transformed.x, transformed.y);
      union_bounds.expand({ static_cast<float>(transformed.x), static_cast<float>(transformed.y) });
    }

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
      path_arc_to(
        cmd.data.add_circle.center,
        cmd.data.add_circle.radius,
        0,
        std::numbers::pi_v<float> * 2.f,
        Viewport_Max_Scale
      );
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
        _add_path_arc_to(
          cmd.data.add_path_arc_to.center,
          cmd.data.add_path_arc_to.p1,
          cmd.data.add_path_arc_to.ccw,
          Viewport_Max_Scale
        );
      break;

    case DrawCmd::Type::add_path_quad_bezier_to:
      if (!_points.empty())
        _add_path_quad_bezier_to(
          cmd.data.add_path_quad_bezier_to.p1,
          cmd.data.add_path_quad_bezier_to.p2,
          Viewport_Max_Scale
        );
      break;

    case DrawCmd::Type::add_path_cubic_bezier_to:
      if (!_points.empty())
        _add_path_cubic_bezier_to(
          cmd.data.add_path_cubic_bezier_to.p1,
          cmd.data.add_path_cubic_bezier_to.p2,
          cmd.data.add_path_cubic_bezier_to.p3,
          Viewport_Max_Scale
        );
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

    case DrawCmd::Type::transform_beg:
      _transform_beg(cmd.data.transform_beg.matrix);
      break;

    case DrawCmd::Type::transform_end:
      _transform_end();
      break;

    case DrawCmd::Type::discard_beg:
    case DrawCmd::Type::discard_end:
      assert(false && "discard operations cannot be nested or crossed");
      std::unreachable();

    default:
      err_if(true, "unsupported draw command inside union");
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

  if (union_thickness > 0)
  {
    if (auto scale = uniform_transform_scale())
      union_thickness *= scale.value();
  }

  if (should_cull(union_bounds, std::abs(union_thickness) + 2.f))
    return;

  auto const cache_vertex_beg = _vertex_beg;
  auto const cache_index_beg  = _index_beg;

  for (auto const& path : Union(subjects, FillRule::NonZero, 2))
  {
    if (path.size() < 3)
      continue;

    _points.reserve(path.size());
    for (auto const& pt : path)
      _points.emplace_back(static_cast<float>(pt.x), static_cast<float>(pt.y));

    if (union_thickness > 0)
      add_poly_line(union_color, union_thickness, true, false, true);
    else if (is_convex(_points))
      add_convex_poly_filled(union_color);
    else
      add_concave_poly_filled(union_color);
  }

  if (cacheable)
  {
    auto cached = CachedUnionGeometry{};
    cached.key       = std::move(cache_key);
    cached.bounds    = union_bounds;
    cached.last_used = ++_union_cache_clock;
    cached.vertices.assign(_vertices.begin() + cache_vertex_beg, _vertices.end());
    cached.indices.reserve(_indices.size() - cache_index_beg);
    for (auto index = cache_index_beg; index < _indices.size(); ++index)
      cached.indices.emplace_back(static_cast<uint16>(_indices[index] - cache_vertex_beg));

    auto const cache_id = beg_cmd.data.union_beg.cache_id;
    if (auto it = _union_cache.find(cache_id); it != _union_cache.end())
    {
      _union_cache_bytes -= it->second.byte_size();
      it->second = std::move(cached);
      _union_cache_bytes += it->second.byte_size();
    }
    else
    {
      auto [inserted_it, inserted] = _union_cache.emplace(cache_id, std::move(cached));
      assert(inserted);
      _union_cache_bytes += inserted_it->second.byte_size();
    }

    while (_union_cache.size() > Union_Cache_Max_Entries || _union_cache_bytes > Union_Cache_Max_Bytes)
    {
      auto const oldest = std::ranges::min_element(_union_cache, {}, [](auto const& pair) { return pair.second.last_used; });
      assert(oldest != _union_cache.end());
      _union_cache_bytes -= oldest->second.byte_size();
      _union_cache.erase(oldest);
    }
  }
}

}
