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
  auto& ctx = get_ctx();
  
  assert(ctx.begining == false);
  ctx.begining = true;

  ctx.layouts.emplace(Layout{ name, pos });

  ctx.states.try_emplace(name.data(), std::vector<Widget>());
  ctx.call_stack.try_emplace(name.data(), std::vector<std::string>());
}

void end()
{
  assert(get_ctx().begining);
  get_ctx().begining = false;
}

void render()
{
  assert(get_ctx().engine);

  auto& ctx = get_ctx();

  while (!ctx.layouts.empty())
  {
    auto& layout = ctx.layouts.front();
    if (layout.shape_infos.empty())
    {
      ctx.layouts.pop();
      break;
    }
    //ctx.engine->update(layout.shape_infos);
    //ctx.engine->render(layout.shape_infos, ctx.window_extent, layout.pos);
    ctx.layouts.pop();
  }

  ctx.call_stack.clear();
}

////////////////////////////////////////////////////////////////////////////////
//                                Shape
////////////////////////////////////////////////////////////////////////////////

void line(glm::vec2 start, glm::vec2 end, uint32_t color, float thickness)
{

}

void polygon(std::vector<glm::vec2> const& points, uint32_t color, float thickness)
{
  assert(points.size() > 2 && thickness >= 0.f);

  //if (thickness == 0.f)
  //{
  //  auto& ctx = get_ctx();
  //  ctx.shape_infos.emplace_back(ShapeInfo
  //  {
  //    .points = points,
  //    .color  = color,
  //    .type   = type::shape::polygon,
  //  });
  //  return;
  //}
  //
  //// TODO:
  //assert(false);
}

////////////////////////////////////////////////////////////////////////////////
//                                UI
////////////////////////////////////////////////////////////////////////////////

auto get_bounding_rectangle(std::vector<glm::vec2> const& data) -> std::pair<glm::vec2, glm::vec2>
{
  assert(data.size() > 2);

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
  // get bounding rectangle
  auto win_pos = get_ctx().layouts.back().pos;
  auto res     = get_bounding_rectangle(data);
  auto min     = res.first  + win_pos;
  auto max     = res.second + win_pos;

  // get mouse pos
  float x, y;
  SDL_GetMouseState(&x, &y);  

  // detect whether mouse on button
  if (x > min.x && y > min.y &&
      x < max.x && y < max.y)
  {
    return true;
  }
  return false;
}

bool click_area(std::string_view name, glm::vec2 const& pos0, glm::vec2 const& pos1)
{
  auto& ctx    = get_ctx();
  auto& layout = ctx.layouts.back();

  // detect whether already have same button
  {
    // get current layout's widgets name
    auto& widgets = ctx.call_stack.at(layout.name.data());
    // have same button throw exception
    auto it = std::find_if(widgets.begin(), widgets.end(), [&](auto const& str) { return str == name; });
    if (it == widgets.end())
      widgets.emplace_back(name.data());
    else
      throw_if(true, "duplication button");
  }

  // get widgets of current layout
  auto& widgets = ctx.states.at(layout.name.data());
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
  if (ctx.event_type == SDL_EVENT_MOUSE_BUTTON_DOWN && detect_mouse_on_button(detect_data))
  {
    widget->first_click = true;
    return false;
  }

  if (widget->first_click && ctx.event_type == SDL_EVENT_MOUSE_BUTTON_UP)
  {
    widget->first_click = false;
    if (detect_mouse_on_button(detect_data))
      return true;
    else
      return false;
  }

  return false;
}

bool button(std::string_view name, type::shape shape, std::vector<glm::vec2> const& data, uint32_t color, float thickness)
{
  auto& ctx    = get_ctx();
  auto& layout = ctx.layouts.back();

  // detect whether already have same button
  {
    // get current layout's widgets name
    auto& widgets = ctx.call_stack.at(layout.name.data());
    // have same button throw exception
    auto it = std::find_if(widgets.begin(), widgets.end(), [&](auto const& str) { return str == name; });
    if (it == widgets.end())
      widgets.emplace_back(name.data());
    else
      throw_if(true, "duplication button");
  }

  // get widgets of current layout
  auto& widgets = ctx.states.at(layout.name.data());
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
  //switch (shape)
  //{
  //case type::shape::triangle:
  //  assert(num == 3);
  //  triangle(data[0], data[1], data[2], color, thickness);
  //  detect_data = data;
  //  break;
  //case type::shape::rectangle:
  //  assert(num == 2);
  //  rectangle(data[0], data[1], color, thickness);
  //  detect_data = { data[0], { data[1].x, data[0].y }, data[1], { data[0].x, data[1].y } };
  //  break;
  //}

  if (ctx.event_type == SDL_EVENT_MOUSE_BUTTON_DOWN && detect_mouse_on_button(detect_data))
  {
    widget->first_click = true;
    return false;
  }

  if (widget->first_click && ctx.event_type == SDL_EVENT_MOUSE_BUTTON_UP)
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