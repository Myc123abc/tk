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

  if (ctx.vertices.empty())
  {
    ctx.layouts = {};
    return;
  }

  // update vertices indices data
  ctx.engine->update(ctx.vertices, ctx.indices);
  ctx.vertices.clear();
  ctx.indices.clear();

  // render every layout
  while (!ctx.layouts.empty())
  {
    auto& layout = ctx.layouts.front();
    if (layout.index_infos.empty())
    {
      ctx.layouts.pop();
      break;
    }
    ctx.engine->render(layout.index_infos, ctx.window_extent, layout.pos);
    ctx.layouts.pop();
  }

  ctx.call_stack.clear();
}

////////////////////////////////////////////////////////////////////////////////
//                                Shape
////////////////////////////////////////////////////////////////////////////////

void rectangle(glm::vec2 const& pos0, glm::vec2 const& pos1, uint32_t color, float thickness)
{
  auto& ctx = get_ctx();
  assert(ctx.begining);

  if (thickness > 0.f)
  {
    path_line_to(pos0);
    path_line_to(pos1.x, pos0.y);
    path_line_to(pos1);
    path_line_to(pos0.x, pos1.y);
    path_stroke(color, thickness, true);
    return;
  }

  uint32_t idx_beg    = ctx.vertices.size();
  uint32_t idx_offset = ctx.indices.size();

  // set vertices
  ctx.vertices.append_range(std::vector<Vertex>
  {
    { pos0,               {}, color },
    { { pos1.x, pos0.y }, {}, color },
    { pos1,               {}, color },
    { { pos0.x, pos1.y }, {}, color },
  });

  // set indices
  ctx.indices.append_range(std::vector<uint32_t>
  {
    idx_beg, idx_beg + 1, idx_beg + 2,
    idx_beg, idx_beg + 2, idx_beg + 3,
  });

  // set index info
  ctx.layouts.back().index_infos.emplace_back(GraphicsEngine::IndexInfo{ idx_offset, 6 });
}

void triangle(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, uint32_t color, float thickness)
{
  assert(get_ctx().begining && thickness >= 0.f);
  
  if (thickness > 0.f)
  {
    path_line_to(p1);
    path_line_to(p2);
    path_line_to(p3);
    path_stroke(color, thickness, true);
    return;
  }

  uint32_t idx_beg    = get_ctx().vertices.size();
  uint32_t idx_offset = get_ctx().indices.size();

  // set vertices
  get_ctx().vertices.append_range(std::vector<Vertex>
  {
    { p1, {}, color },
    { p2, {}, color },
    { p3, {}, color },
  });

  // set indices
  get_ctx().indices.append_range(std::vector<uint32_t>
  {
    idx_beg, idx_beg + 1, idx_beg + 2,
  });

  // set index info
  get_ctx().layouts.back().index_infos.emplace_back(GraphicsEngine::IndexInfo{ idx_offset, 3 });
}

void path_line_to(glm::vec2 point)
{
  get_ctx().points.emplace_back(point);
}

inline auto get_line_normal(glm::vec2 const& a, glm::vec2 const& b) -> glm::vec2
{
  auto d   = b - a;
  auto len = glm::length(d);
  assert(len > 0);
  return { -d.y / len, d.x / len };
}

inline auto get_point_normal(glm::vec2 const& a, glm::vec2 const& b) -> glm::vec2
{
  auto c =  (a + b) / 2.f;
  return glm::normalize(c);
}

inline auto get_theta(glm::vec2 const& a, glm::vec2 const& b) -> float
{
  return glm::degrees(glm::acos(glm::dot(a, b) / glm::length(a) / glm::length(b)));
}

inline auto get_inline_point_offset(float thickness, float theta) -> float
{
  return thickness / glm::sin(glm::radians(theta / 2));
}

void path_stroke(uint32_t color, float thickness, bool is_closed)
{
  // promise is use ui::begin()
  assert(get_ctx().begining          &&
  // points at least have two
         get_ctx().points.size() > 2 &&
  // thickness at least 1.f
         thickness >= 1.f);

  // get points and point count
  auto& points = get_ctx().points;
  auto point_count = points.size();

  // get vertices and indices
  auto& vertices = get_ctx().vertices;
  auto& indices  = get_ctx().indices;

  // get begin of index and offset of index
  // for current shape
  uint32_t idx_beg    = vertices.size();
  uint32_t idx_offset = indices.size();

  // TODO: 
  // 1. Tickness anti-aliased lines cap are missing their AA fringe.
  // 2. For unclosed, line normals only have line_count number
  // 3. If line is not closed, the first and last points need to be generated differently as there are no normals to blend
  

  // if is a closed shape, the line count equal the point count
  // if is not a closed shape, lint count will be reduced one
  if (is_closed)
  {
    // get line normals
    // if we draw a triangle stroke
    // in clockwise it have l0, l1, l2
    // we will get line normals ln0, ln1, ln2
    auto line_count   = point_count;
    auto line_normals = std::vector<glm::vec2>();
    line_normals.reserve(line_count);
    for (auto i = 0; i < line_count; ++i)
    {
      auto j = i + 1 == line_count ? 0 : i + 1;
      line_normals.emplace_back(get_line_normal(points[i], points[j]));
    }

    // get point normals
    // also in triangle stroke
    // we from line normals to get point normals
    // from ln0, ln1, ln2 we get pn1, pn2, pn0
    auto point_normals = std::vector<glm::vec2>();
    point_normals.reserve(point_count);
    for (auto i = 0; i < point_count; ++i)
    {
      auto j = i + 1 == point_count ? 0 : i + 1;
      point_normals.emplace_back(get_point_normal(line_normals[i], line_normals[j]));
    }

    // get thetas
    auto thetas = std::vector<float>();
    thetas.reserve(line_count);
    for (auto i = 0; i < point_count; ++i)
    {
      uint32_t j = i + 1, k = i + 2;
      if (i + 2 == point_count)
      {
        j = i + 1;
        k = 0;
      }
      else if (i + 1 == point_count)
      {
        j = 0;
        k = 1;
      }
      thetas.emplace_back(get_theta(points[i] - points[j], points[k] - points[j]));
    }

    // get internal points
    // result is ip1, ip2, ip0
    auto internal_points = std::vector<glm::vec2>();
    internal_points.reserve(point_count);
    for (auto i = 0; i < point_count; ++i)
    {
      auto j = i + 1 == point_count ? 0 : i + 1;
      internal_points.emplace_back(point_normals[i] * get_inline_point_offset(thickness, thetas[i]));
    }
    // HACK: the bad way, performance problem, need to combine multi-for to single
    internal_points.insert(internal_points.begin(), internal_points.back());
    internal_points.pop_back();
    for (auto i = 0; i < point_count; ++i)
      internal_points[i] += points[i];

    // add vertices
    // every point need a extre point to contruct a line (have thickness rectangle)
    auto vertex_count = point_count * 2;
    vertices.reserve(idx_beg + vertex_count);
    for (auto i = 0; i < point_count; ++i)
    {
      vertices.append_range(std::vector<Vertex>
      { 
        { points[i],          {}, color },
        { internal_points[i], {}, color },
      });
    }

    // add indices
    // one line need 6 indices to draw a rectangle
    uint32_t index_count = line_count * 6;
    indices.reserve(idx_offset + index_count);
    auto beg = idx_beg;
    for (auto i = 0; i < line_count - 1; ++i)
    {
      indices.append_range(std::vector<uint32_t>
      {
        idx_beg,     idx_beg + 2, idx_beg + 1,
        idx_beg + 1, idx_beg + 2, idx_beg + 3,
      });
      idx_beg += 2;
    }
    indices.append_range(std::vector<uint32_t>
    {
      idx_beg,     beg, idx_beg + 1,
      idx_beg + 1, beg, beg     + 1,
    });

    // add index info
    get_ctx().layouts.back().index_infos.emplace_back(GraphicsEngine::IndexInfo{ idx_offset, index_count });
  }
  else
  {
    // TODO: do unclosed case
    assert(false);
  }

  points.clear();
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
  switch (shape)
  {
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
  }

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