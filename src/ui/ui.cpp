#include "tk/ui/ui.hpp"
#include "internal.hpp"
#include "tk/ErrorHandling.hpp"

#include <cassert>

using namespace tk::graphics_engine;

namespace tk { namespace ui {

////////////////////////////////////////////////////////////////////////////////
//                                  Misc
////////////////////////////////////////////////////////////////////////////////

// FIXME: discard?
auto generate_id() -> uint32_t
{
  static uint32_t id = 0;
  assert(id < UINT32_MAX);
  return ++id;
}

void begin(std::string_view name, glm::vec2 const& pos)
{
  auto ctx = get_ctx();
  
  assert(ctx->begining == false);
  ctx->begining = true;

  ctx->layouts.emplace(Layout{ name, pos });

  ctx->states.try_emplace(name.data(), std::vector<Widget>());
  ctx->call_stack.try_emplace(name.data(), std::vector<std::string>());
}

void end()
{
  assert(get_ctx()->begining);
  get_ctx()->begining = false;
}

void render()
{
  auto ctx = get_ctx();
  if (ctx->shape_infos.empty()) return;
  assert(ctx->engine && ctx->shape_infos.back().op == type::shape_op::mix);

  ctx->engine->sdf_update(ctx->points, ctx->shape_infos);
  ctx->engine->sdf_render(ctx->points.size() * 2, ctx->shape_infos.size());
  ctx->points.clear();
  ctx->shape_infos.clear();

  while (!ctx->layouts.empty()) ctx->layouts.pop();
  ctx->call_stack.clear();
}

void text_mask_render()
{
  auto ctx = get_ctx();
  ctx->engine->text_mask_render(ctx->a, ctx->p);
}

////////////////////////////////////////////////////////////////////////////////
//                                Shape
////////////////////////////////////////////////////////////////////////////////

auto convert_color_format(uint32_t color)
{
  float r = float((color >> 24) & 0xFF) / 255;
  float g = float((color >> 16) & 0xFF) / 255;
  float b = float((color >> 8 ) & 0xFF) / 255;
  float a = float((color      ) & 0xFF) / 255;
  return glm::vec4(r, g, b, a);
}

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
  auto& pos = ctx->layouts.back().pos;

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
    .color     = convert_color_format(color),
    .thickness = thickness,
  });
}

void circle(glm::vec2 const& center, float radius, uint32_t color, uint32_t thickness)
{
  auto ctx = get_ctx();
  throw_if(!ctx->begining || ctx->path_begining,
           "failed to draw circle, need ui::begin() or cannot draw in ui::path_begin()");
  
  auto& pos = ctx->layouts.back().pos;

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
    .color     = convert_color_format(color),
    .thickness = thickness,
  });
}

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
  info.color     = convert_color_format(color);
  info.thickness = thickness;
  info.num       = ctx->shape_infos.size() - ctx->path_idx - 1;
  assert(info.num != 0);
}

void text(std::string_view text, glm::vec2 const& pos, float size, uint32_t color)
{
  auto ctx = get_ctx();
  auto res = ctx->engine->parse_text(text, pos, size);
  ctx->a = res.first;
  ctx->p = res.second;
  shape(type::shape::glyph, { glm::vec2(res.second.x, res.second.y), glm::vec2(res.second.z, res.second.w) }, color);
}

////////////////////////////////////////////////////////////////////////////////
//                                UI
////////////////////////////////////////////////////////////////////////////////

void set_operation(type::shape_op op)
{
  auto ctx = get_ctx();
  assert(ctx->begining);
  ctx->shape_infos.back().op = op;
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

bool detect_mouse_on_button(std::vector<glm::vec2> const& data)
{
  auto ctx = get_ctx();
  // get bounding rectangle
  auto win_pos = get_ctx()->layouts.back().pos;
  auto res     = get_bounding_rectangle(data);
  auto min     = res.first  + win_pos;
  auto max     = res.second + win_pos;

  // get mouse pos
  auto cursor_pos = ctx->window->get_cursor_position();

  // detect whether mouse on button
  if (cursor_pos.x > min.x && cursor_pos.y > min.y &&
      cursor_pos.x < max.x && cursor_pos.y < max.y)
  {
    return true;
  }
  return false;
}

bool click_area(std::string_view name, glm::vec2 const& pos0, glm::vec2 const& pos1)
{
  auto ctx    = get_ctx();
  auto& layout = ctx->layouts.back();

  // detect whether already have same button
  {
    // get current layout's widgets name
    auto& widgets = ctx->call_stack.at(layout.name.data());
    // have same button throw exception
    auto it = std::find_if(widgets.begin(), widgets.end(), [&](auto const& str) { return str == name; });
    if (it == widgets.end())
      widgets.emplace_back(name.data());
    else
      throw_if(true, "duplication button");
  }

  // get widgets of current layout
  auto& widgets = ctx->states.at(layout.name.data());
  auto it = std::find_if(widgets.begin(), widgets.end(), [&](auto const& widget) { return widget.name == name; });

  Widget* widget{};

  // not found, create the new widget
  if (it == widgets.end())
  {
    widgets.emplace_back(Widget
    {
      .name        = name.data(),
      .id          = generate_id(),
      .first_click = false,
    });
    widget = &widgets.back();
  }
  else
    widget = &*it;

  auto detect_data = { pos0, { pos1.x, pos0.y }, pos1, { pos0.x, pos1.y } };
  auto mouse_state = ctx->window->get_mouse_state();
  if (mouse_state == type::mouse::left_down && detect_mouse_on_button(detect_data))
  {
    widget->first_click = true;
    return false;
  }

  if (widget->first_click && mouse_state == type::mouse::left_up)
  {
    widget->first_click = false;
    if (detect_mouse_on_button(detect_data))
      return true;
    else
      return false;
  }

  return false;
}

bool button(std::string_view name, type::shape shape, std::vector<glm::vec2> const& data, uint32_t color, uint32_t thickness)
{
  auto ctx    = get_ctx();
  auto& layout = ctx->layouts.back();

  // detect whether already have same button
  {
    // get current layout's widgets name
    auto& widgets = ctx->call_stack.at(layout.name.data());
    // have same button throw exception
    auto it = std::find_if(widgets.begin(), widgets.end(), [&](auto const& str) { return str == name; });
    if (it == widgets.end())
      widgets.emplace_back(name.data());
    else
      throw_if(true, "duplication button");
  }

  // get widgets of current layout
  auto& widgets = ctx->states.at(layout.name.data());
  auto it = std::find_if(widgets.begin(), widgets.end(), [&](auto const& widget) { return widget.name == name; });

  Widget* widget{};

  // not found, create the new widget
  if (it == widgets.end())
  {
    widgets.emplace_back(Widget
    {
      .name        = name.data(),
      .id          = generate_id(),
      .first_click = false,
    });
    widget = &widgets.back();
  }
  else
    widget = &*it;

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
  case type::shape::glyph:
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

  auto mouse_state = ctx->window->get_mouse_state();
  if (mouse_state == type::mouse::left_down && detect_mouse_on_button(detect_data))
  {
    widget->first_click = true;
    return false;
  }

  if (widget->first_click && mouse_state == type::mouse::left_up)
  {
    widget->first_click = false;
    if (detect_mouse_on_button(detect_data))
      return true;
    else
      return false;
  }

  return false;
}

}}