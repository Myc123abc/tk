#include "tk/ui/ui.hpp"
#include "internal.hpp"
#include "tk/ErrorHandling.hpp"

#include <cassert>

using namespace tk::graphics_engine;

namespace tk { namespace ui {

////////////////////////////////////////////////////////////////////////////////
//                                  Misc
////////////////////////////////////////////////////////////////////////////////

void begin(std::string_view name, glm::vec2 const& pos)
{
  auto ctx = get_ctx();
  
  assert(ctx->begining == false);
  ctx->begining = true;

  ctx->layouts.emplace_back(Layout
  {
    .name = name.data(),
    .pos  = pos,
  });
  ctx->last_layout = std::to_address(ctx->layouts.rbegin());
}

void end()
{
  assert(get_ctx()->begining);
  get_ctx()->begining = false;
}

auto get_bounding_rectangle(std::vector<glm::vec2> const& data) -> std::pair<glm::vec2, glm::vec2>
{
  assert(data.size() > 1);

  glm::vec2 min = data[0];
  glm::vec2 max = data[0];

  for (auto i = 1; i < data.size(); ++i)
  {
    auto& p = data[i];
    if (p.x < min.x) min.x = p.x;
    if (p.y < min.y) min.y = p.y;
    if (p.x > max.x) max.x = p.x;
    if (p.y > max.y) max.y = p.y;
  }

  return { min, max };
}

bool hit(glm::vec2 const& pos, std::vector<glm::vec2> const& data)
{
  if (data.empty()) return false;

  // get bounding rectangle
  auto win_pos = get_ctx()->last_layout->pos;
  auto res     = get_bounding_rectangle(data);
  auto min     = res.first  + win_pos;
  auto max     = res.second + win_pos;

  // detect whether mouse on button
  if (pos.x > min.x && pos.y > min.y &&
      pos.x < max.x && pos.y < max.y)
    return true;
  return false;
}

void clear()
{
  auto ctx = get_ctx();

  ctx->vertices.clear();
  ctx->indices.clear();
  ctx->index = {};
  ctx->shape_properties.clear();
  ctx->shape_offset = {};

  if (hit(ctx->mouse_pos, ctx->current_hovered_widget_rect))
    ctx->last_hovered_widget = ctx->current_hovered_widget;
  else
    ctx->last_hovered_widget = {};

  // clear frame resources
  ctx->points.clear();
  ctx->shape_infos.clear();
  ctx->layouts.clear();
  ctx->click_finish = {};
}

void render()
{
  auto ctx = get_ctx();
  //if (ctx->shape_infos.empty()) return;
  //assert(ctx->engine && ctx->shape_infos.back().op == type::shape_op::mix);
  assert(ctx->engine);
  //ctx->engine->sdf_update(ctx->points, ctx->shape_infos);
  //ctx->engine->sdf_render(ctx->points.size() * 2, ctx->shape_infos.size());
  ctx->engine->sdf_render(ctx->vertices, ctx->indices, ctx->shape_properties);
  
  clear();
}

void text_mask_render()
{
  auto ctx = get_ctx();
  ctx->engine->text_mask_render();
}

////////////////////////////////////////////////////////////////////////////////
//                               Draw Shape
////////////////////////////////////////////////////////////////////////////////

void shape(type::shape type, std::vector<float> const& values, uint32_t color, uint32_t thickness, std::pair<glm::vec2, glm::vec2> const& box)
{
  auto ctx = get_ctx();
  
  auto& pos = ctx->last_layout->pos;
  auto  min = pos + box.first  - glm::vec2(1);
  auto  max = pos + box.second + glm::vec2(1);

  ctx->vertices.reserve(ctx->vertices.size() + 4);
  ctx->vertices.append_range(std::vector<Vertex>
  {
    { min,              {}, color, ctx->shape_offset },
    { { max.x, min.y }, {}, color, ctx->shape_offset },
    { max,              {}, color, ctx->shape_offset },
    { { min.x, max.y }, {}, color, ctx->shape_offset },
  });

  ctx->indices.reserve(ctx->indices.size() + 6);
  ctx->indices.append_range(std::vector<uint16_t>
  {
    static_cast<uint16_t>(ctx->index + 0), static_cast<uint16_t>(ctx->index + 1), static_cast<uint16_t>(ctx->index + 3),
    static_cast<uint16_t>(ctx->index + 3), static_cast<uint16_t>(ctx->index + 1), static_cast<uint16_t>(ctx->index + 2),
  });
  ctx->index += 4;

  ctx->shape_properties.emplace_back(type, thickness);
  ctx->shape_properties.back().values.append_range(values);

  ctx->shape_offset += 2 + values.size();
}

void line(glm::vec2 const& p0, glm::vec2 const& p1, uint32_t color)
{
  if (p0 != p1) shape(type::shape::line, { p0.x, p0.y, p1.x, p1.y }, color, 0, { p0, p1 });
}

void rectangle(glm::vec2 const& left_top, glm::vec2 const& right_bottom, uint32_t color, uint32_t thickness)
{
  shape(type::shape::rectangle, { left_top.x, left_top.y, right_bottom.x, right_bottom.y }, color, thickness, { left_top, right_bottom });
}

void triangle(glm::vec2 const& p0, glm::vec2 const& p1, glm::vec2 const& p2, uint32_t color, uint32_t thickness)
{
  shape(type::shape::triangle, { p0.x, p0.y, p1.x, p1.y, p2.x, p2.y }, color, thickness, get_bounding_rectangle({ p0, p1, p2 }));
}

void polygon(std::vector<glm::vec2> const& points, uint32_t color, uint32_t thickness)
{
  std::vector<float> data;
  data.reserve(1 + points.size() * 2);
  data.emplace_back(std::bit_cast<float>(static_cast<uint32_t>(points.size())));
  for (auto const& point : points)
    data.append_range(std::vector<float>{ point.x, point.y });
  shape(type::shape::polygon, data, color, thickness, get_bounding_rectangle(points));
}

void circle(glm::vec2 const& center, float radius, uint32_t color, uint32_t thickness)
{
  shape(type::shape::circle, { center.x, center.y, radius }, color, thickness, { center - radius, center + radius });
}

void bezier(glm::vec2 const& p0, glm::vec2 const& p1, glm::vec2 const& p2, uint32_t color)
{
  shape(type::shape::bezier, { p0.x, p0.y, p1.x, p1.y, p2.x, p2.y }, color, 0, get_bounding_rectangle({ p0, p1, p2 }));
}

#if 0
void shape(type::shape type, std::vector<glm::vec2> const& points, uint32_t color, uint32_t thickness)
{
  using enum type::shape;

  auto ctx = get_ctx();

  // convert type when use path
  if (ctx->path_begining)
  {
    if (type == line)
    {
      type = line_partition;
    }
    else if (type == bezier)
    {
      type = bezier_partition;
    }
  }

  // promise use ui::begin() and ui::path_begin() if use
  throw_if(!ctx->begining, "failed to draw shape, need ui::begin()");
  if (ctx->path_begining)
    throw_if(type != line_partition && type != bezier_partition, "failed to draw path, only support line and bezier");
  
  // get layout position
  auto& pos = ctx->last_layout->pos;

  // start index of points
  uint32_t offset = ctx->points.size() * 2;

  // add points
  ctx->points.reserve(ctx->points.size() + points.size());
  for (auto& point : points) 
    ctx->points.emplace_back(pos + point);

  // add shape info
  ctx->shape_infos.emplace_back(ShapeInfo
  {
    .type      = type,
    .offset    = offset,
    .num       = static_cast<uint32_t>(points.size()),
    //.color     = convert_color_format(color),
    .thickness = thickness,
  });
}

void circle2(glm::vec2 const& center, float radius, uint32_t color, uint32_t thickness)
{
  auto ctx = get_ctx();
  throw_if(!ctx->begining || ctx->path_begining,
           "failed to draw circle, need ui::begin() or cannot draw in ui::path_begin()");
  
  auto& pos = ctx->last_layout->pos;

  uint32_t offset = ctx->points.size() * 2;

  ctx->points.reserve(ctx->points.size() + 2);
  ctx->points.append_range(std::vector<glm::vec2>
  {
    pos + center,
    glm::vec2(radius), // TODO: just for simply, store vec2(radius) rather than radius because ctx->points is vector<vec2>
  });

  // add shape info
  ctx->shape_infos.emplace_back(ShapeInfo
  {
    .type      = type::shape::circle,
    .offset    = offset,
    .num       = 2,
    //.color     = convert_color_format(color),
    .thickness = thickness,
  });
}
#endif

void path_begin()
{
  auto ctx = get_ctx();
  assert(ctx->begining && ctx->path_begining == false);
  ctx->path_begining = true;

  ctx->path_idx = ctx->shape_infos.size();

  ctx->shape_infos.emplace_back(ShapeInfo
  {
    .type = type::shape::path,
  });
}

void path_end(uint32_t color, uint32_t thickness)
{
  auto ctx = get_ctx();
  assert(ctx->begining && ctx->path_begining);
  ctx->path_begining = false;

  auto& info     = ctx->shape_infos[ctx->path_idx];
  //info.color     = convert_color_format(color);
  info.thickness = thickness;
  info.num       = ctx->shape_infos.size() - ctx->path_idx - 1;
  assert(info.num != 0);
}

void text(std::string_view text, glm::vec2 const& pos, float size, uint32_t color)
{
  auto ctx    = get_ctx();
  //auto extent = ctx->engine->parse_text(text, pos, size, convert_color_format(color));
  //shape(type::shape::text, { extent.first, extent.second });
}

void set_operation(type::shape_op op)
{
  auto ctx = get_ctx();
  assert(ctx->begining);
  ctx->shape_infos.back().op = op;
}

////////////////////////////////////////////////////////////////////////////////
//                             Mouse Operation
////////////////////////////////////////////////////////////////////////////////

void event_process()
{
  using enum type::mouse_state;

  auto ctx = get_ctx();

  ctx->mouse_pos = ctx->window->get_mouse_position();

  // mouse click
  auto mouse_state = ctx->window->get_mouse_state();
  static bool first_down{};
  if (!first_down && mouse_state == left_down)
  {
    ctx->drag_start_pos = ctx->mouse_pos;
    first_down          = true;
  }
  else if (first_down && mouse_state == left_up)
  {
    ctx->drag_end_pos = ctx->mouse_pos;
    ctx->click_finish = true;
    first_down        = false;
  }
}

auto add_widget(std::string_view name)
{
  auto& widgets = get_ctx()->last_layout->widgets;

  // promise widget name is unique for per layout
  auto it = std::find_if(widgets.begin(), widgets.end(), [&](auto const& widget) { return widget.name == name; });
  assert(it == widgets.end());
  widgets.emplace_back(Widget{ name.data() });

  return std::to_address(widgets.rbegin());
}

auto update_current_hovered_widget(std::string_view name, std::vector<glm::vec2> const& rect)
{
  auto ctx = get_ctx();
  if (hit(ctx->mouse_pos, rect))
  {
    ctx->current_hovered_widget.first  = ctx->last_layout->name;
    ctx->current_hovered_widget.second = name;
    ctx->current_hovered_widget_rect   = rect;
  }
}

bool is_clicked(std::string_view name, std::vector<glm::vec2> const& data)
{
  add_widget(name);
  auto ctx = get_ctx();
  update_current_hovered_widget(name, data);
  if (ctx->current_hovered_widget != ctx->last_hovered_widget)
    return false;
  return ctx->click_finish ? hit(ctx->drag_start_pos, data) &&
                             hit(ctx->drag_end_pos,   data)
                           : false;
}

bool click_area(std::string_view name, glm::vec2 const& pos0, glm::vec2 const& pos1)
{
  return is_clicked(name, { pos0, pos1 });
}

bool button(std::string_view name, type::shape shape, std::vector<glm::vec2> const& data, uint32_t color, uint32_t thickness)
{
  // draw shape
  auto num = data.size();
  std::vector<glm::vec2> detect_data;
  switch (shape)
  {
  case type::shape::line:
  case type::shape::bezier:
  case type::shape::path:
  case type::shape::line_partition:
  case type::shape::bezier_partition:
  case type::shape::text:
    throw_if(false, "this type cannot use on button, please use ui::clickarea");

  case type::shape::triangle:
    assert(num == 3);
    triangle(data[0], data[1], data[2], color, thickness);
    detect_data = data;
    break;
    
  case type::shape::rectangle:
    assert(num == 2);
    rectangle(data[0], data[1], color, thickness);
    detect_data = { data[0], { data[1].x, data[0].y }, data[1], { data[0].x, data[1].y } };
    break;
  
  case type::shape::polygon:
    assert(num > 2);
    polygon(data, color, thickness);
    detect_data = data;
    break;

  case type::shape::circle:
    assert(num == 2);
    circle(data[0], data[1].x, color, thickness);
    detect_data = { data[0] - data[1], data[0] + data[1] };
    break;
  }

  return is_clicked(name, detect_data);
}

bool is_hover_on(std::string_view name)
{
  auto ctx = get_ctx();
  return ctx->last_hovered_widget.first  == ctx->last_layout->name &&
         ctx->last_hovered_widget.second == name;
}

}}