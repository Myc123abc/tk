#include "renderer.hpp"
#include "../../ui/command_list.hpp"
#include "util/error_handling.hpp"

#include <assert.h>

namespace {

auto get_bounding_rectangle(std::vector<glm::vec2> const& data) noexcept -> std::pair<glm::vec2, glm::vec2>
{
  assert(data.size() > 1);

  auto min = data[0];
  auto max = data[0];

  for (auto i = 1; i < data.size(); ++i)
  {
    auto& p = data[i];
    if (p.x < min.x) min.x = p.x;
    if (p.y < min.y) min.y = p.y;
    if (p.x > max.x) max.x = p.x;
    if (p.y > max.y) max.y = p.y;
  }

  if (min.x == max.x || min.y == max.y)
  {
    // promise horizontal or vertical line only 1px
    min -= 0.5;
    max += 0.5;
  }
  else
  {
    min -= 1.0;
    max += 1.0;
  }

  return { min, max };
}

auto is_integer_scale(float s)
{
  return std::fabs(s - std::round(s)) < 0.0001f;
}

}

namespace tk::renderer {

void Renderer::generate_render_data(HWND handle, ui::CommandList* cmd) noexcept
{
  auto& render_data = _render_datas.at(handle);
  render_data.clear();
  _shape_properties_offset = 0;
  _idx_beg                 = 0;

  auto add_scissor = false;
  auto try_push_draw_data = [&]
  {
    if (add_scissor)
    {
      add_scissor = false;
      render_data.push_draw_data();
    }
  };

  while (auto res = cmd->pop())
  {
    auto [type, info_ptr] = res.value();

    using namespace tk::ui;
    using enum CommandType;

    render_data.try_push_draw_data(type);

    switch (type)
    {
    case draw_rectangle:
    {
      try_push_draw_data();
      auto info = reinterpret_cast<RectangleInfo*>(info_ptr);
      auto lt = info->left_top;
      auto rb = info->right_bottom;
      if (info->thickness > 1)
      {
        auto t = info->thickness / 2;
        lt -= t;
        rb += t;
      }
	    add_shape(render_data, ShapeProperty::Type::rectangle, info->color, info->thickness,
        { info->left_top.x, info->left_top.y, info->right_bottom.x, info->right_bottom.y },
        { lt, rb });
    }
    break;

    case draw_triangle:
    {
      try_push_draw_data();
      auto info = reinterpret_cast<TriangleInfo*>(info_ptr);
	    add_shape(render_data, ShapeProperty::Type::triangle, info->color, info->thickness,
        { info->p0.x, info->p0.y, info->p1.x, info->p1.y, info->p2.x, info->p2.y },
        get_bounding_rectangle({ info->p0, info->p1, info->p2 }));
    }
    break;

    case draw_circle:
    {
      try_push_draw_data();
      auto info = reinterpret_cast<CircleInfo*>(info_ptr);
      auto r = info->radius - 1;
      if (r < 0) r = 1;
      add_shape(render_data, ShapeProperty::Type::circle, info->color, info->thickness,
        { info->center.x, info->center.y, r },
        { info->center - info->radius, info->center + info->radius });
    }
    break;

    case draw_line:
    {
      try_push_draw_data();
      auto info = reinterpret_cast<LineInfo*>(info_ptr);
      if (_path_data.empty())
      {
        if (info->thickness > 1)
        {
          auto p  = (info->p0 + info->p1) / 2.f;
          auto dp = glm::abs(info->p1 - info->p0) / 2.f;
          auto t  = info->thickness / 2;
          add_shape(render_data, ShapeProperty::Type::line, info->color, info->thickness,
            { info->p0.x, info->p0.y, info->p1.x, info->p1.y },
            get_bounding_rectangle({ p - dp - t, p + dp + t }));
        }
        else
          add_shape(render_data, ShapeProperty::Type::line, info->color, info->thickness,
            { info->p0.x, info->p0.y, info->p1.x, info->p1.y },
            get_bounding_rectangle({ info->p0, info->p1 }));
      }
      else
      {
        _path_data[0] = std::bit_cast<float>(std::bit_cast<uint32_t>(_path_data[0]) + 1);
        auto points = { info->p0, info->p1 };
        _path_points.append_range(points);
        _path_data.emplace_back(std::bit_cast<float>(ShapeProperty::Type::path_line));
        _path_data.append_range(std::ranges::to<std::vector<float>>(points
          | std::views::transform([](auto const& p) { return std::array<float, 2>{ p.x, p.y }; })
          | std::views::join));
      }
    }
    break;

    case draw_bezier:
    {
      try_push_draw_data();
      auto info = reinterpret_cast<BezierInfo*>(info_ptr);
      if (_path_data.empty())
        add_shape(render_data, ShapeProperty::Type::bezier, info->color, info->thickness,
          { info->p0.x, info->p0.y, info->p1.x, info->p1.y, info->p2.x, info->p2.y },
          get_bounding_rectangle({ info->p0, info->p1, info->p2 }));
      else
      {
        _path_data[0] = std::bit_cast<float>(std::bit_cast<uint32_t>(_path_data[0]) + 1);
        auto points = { info->p0, info->p1, info->p2 };
        _path_points.append_range(points);
        _path_data.emplace_back(std::bit_cast<float>(ShapeProperty::Type::path_bezier));
        _path_data.append_range(std::ranges::to<std::vector<float>>(points
          | std::views::transform([](auto const& p) { return std::array<float, 2>{ p.x, p.y }; })
          | std::views::join));
      }
    }
    break;

    case add_discard_rectangle:
    {
      try_push_draw_data();
      auto info = reinterpret_cast<DiscardRectangleInfo*>(info_ptr);

      err_if(render_data.shape_properties.empty(), "failed must draw a shape then use discard rectangle");
      render_data.shape_properties.back().set_operator(ShapeProperty::Operator::discard);

      add_shape_property(render_data, ShapeProperty::Type::rectangle, {}, {},
        { info->left_top.x, info->left_top.y, info->right_bottom.x, info->right_bottom.y });
    }
    break;

    case begin_path:
    {
      try_push_draw_data();
      // record count
      _path_data.push_back(std::bit_cast<float>(0u));
    }
    break;

    case end_path:
    {
      auto info = reinterpret_cast<EndPathInfo*>(info_ptr);

      err_if(_path_data.empty(), "path drawing not have any data");
      add_shape(render_data, ShapeProperty::Type::path, info->color, info->thickness, _path_data, get_bounding_rectangle(_path_points));
      _path_data.clear();
      _path_points.clear();
    }
    break;

    case begin_union:
    {
      try_push_draw_data();
      _op_data.op     = ShapeProperty::Operator::u;
      _op_data.offset = _shape_properties_offset;
    }
    break;

    case end_union:
    {
      auto info = reinterpret_cast<EndUnionInfo*>(info_ptr);

      render_data.shape_properties.back().set_color(_tmp_color.value_or(info->color));
      render_data.shape_properties.back().set_thickness(info->thickness);
      render_data.shape_properties.back().set_operator({});

      add_vertices_indices(render_data, get_bounding_rectangle(_op_data.points));

      _op_data.op     = {};
      _op_data.offset = {};
      _op_data.points.clear();
    }
    break;

    case enable_tmp_color:
    {
      auto info = reinterpret_cast<TmpColorInfo*>(info_ptr);
      _tmp_color = info->color;
    }
    break;

    case disable_tmp_color:
    {
      _tmp_color = {};
    }
    break;

    case image:
    {
      auto info = reinterpret_cast<ImageInfo*>(info_ptr);
      if (_images.contains(info->handle))
      {
        add_vertices_indices(render_data, { info->left_top, info->right_bottom });
        render_data.set_image_info(&_images.at(info->handle), static_cast<float>(info->alpha) / 0xff);
      }
    }
    break;

    case set_scissor_rect:
    {
      add_scissor = true;
      auto info = reinterpret_cast<ScissorRectInfo*>(info_ptr);
      render_data.push_scissor_rect(info->rect);
    }
    break;

    case set_window_pos:
    {
      auto info = reinterpret_cast<WindowPosInfo*>(info_ptr);
      render_data.resizing_window_pos = info->pos;
    }
    break;

    case draw_window_shadow:
    {
      auto info = reinterpret_cast<WindowShadowInfo*>(info_ptr);
      render_data.window_shadow_info = *info;
    }
    break;

    }
  }

  render_data.generate_finish();

  cmd->notify();
}

void Renderer::add_vertices_indices(RenderData& render_data, std::pair<glm::vec2, glm::vec2> bounding_rectangle) noexcept
{
  auto [min, max] = bounding_rectangle;

  auto offset = _op_data.op == ShapeProperty::Operator::none ? _shape_properties_offset : _op_data.offset;

  render_data.vertices.append_range(std::vector<Vertex>
  {
    { { min.x, min.y, 0.f }, { 0.f, 0.f }, offset },
    { { max.x, min.y, 0.f }, { 1.f, 0.f }, offset },
    { { max.x, max.y, 0.f }, { 1.f, 1.f }, offset },
    { { min.x, max.y, 0.f }, { 0.f, 1.f }, offset },
  });
  render_data.indices.append_range(std::vector<uint16_t>
  {
    static_cast<uint16_t>(_idx_beg + 0),
    static_cast<uint16_t>(_idx_beg + 1),
    static_cast<uint16_t>(_idx_beg + 2),
    static_cast<uint16_t>(_idx_beg + 0),
    static_cast<uint16_t>(_idx_beg + 2),
    static_cast<uint16_t>(_idx_beg + 3),
  });
  _idx_beg += 4;
}

void Renderer::add_shape_property(RenderData& render_data, ShapeProperty::Type type, glm::vec4 color, float thickness, std::vector<float> const& values) noexcept
{
  render_data.shape_properties.emplace_back(ShapeProperty
  {
    type,
    _tmp_color.value_or(color),
    thickness,
    _op_data.op,
    values
  });
  _shape_properties_offset += render_data.shape_properties.back().byte_size();
}

void Renderer::add_shape(RenderData& render_data, ShapeProperty::Type type, glm::vec4 color, float thickness, std::vector<float> const& values, std::pair<glm::vec2, glm::vec2> bounding_rectangle) noexcept
{
  auto [min, max] = bounding_rectangle;

  if (_op_data.op == ShapeProperty::Operator::u)
  {
    _op_data.points.emplace_back(min);
    _op_data.points.emplace_back(max);
    goto add_shape_property;
  }

  add_vertices_indices(render_data, bounding_rectangle);
add_shape_property:
  add_shape_property(render_data, type, color, thickness, values);
}

}
