#include "tk/ui/ui.hpp"
#include "internal.hpp"

#include <cassert>
#include <algorithm>

using namespace tk::graphics_engine;

namespace tk { namespace ui {

void begin(glm::vec2 const& pos)
{
  assert(get_ctx().using_layout == false);
  get_ctx().using_layout = true;
  get_ctx().layouts.push({ pos });
}

void end()
{
  assert(get_ctx().using_layout);
  get_ctx().using_layout = false;
}

void render()
{
  assert(get_ctx().engine);

  auto& ctx = get_ctx();

  // update vertices indices data
  ctx.engine->update(ctx.vertices, ctx.indices);
  ctx.vertices.clear();
  ctx.indices.clear();

  // render every layout
  while (!ctx.layouts.empty())
  {
    auto& layout = ctx.layouts.back();
    ctx.engine->render(layout.index_infos, ctx.window_extent, layout.pos);
    ctx.layouts.pop();
  }
}

void rectangle(glm::vec2 const& pos, glm::vec2 const& extent, uint32_t color)
{
  assert(get_ctx().using_layout);

  auto lower_down     = pos + extent;
  uint32_t idx_beg    = get_ctx().vertices.size();
  uint32_t idx_offset = get_ctx().indices.size();

  // set vertices
  get_ctx().vertices.append_range(std::vector<Vertex>
  {
    { pos,                     {}, color },
    { { lower_down.x, pos.y }, {}, color },
    { lower_down,              {}, color },
    { { pos.x, lower_down.y }, {}, color },
  });

  // set indices
  get_ctx().indices.append_range(std::vector<uint32_t>
  {
    idx_beg, idx_beg + 1, idx_beg + 2,
    idx_beg, idx_beg + 2, idx_beg + 3,
  });

  // set index info
  get_ctx().layouts.back().index_infos.emplace_back(GraphicsEngine::IndexInfo{ idx_offset, 6 });
}

void triangle(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, uint32_t color, float thickness)
{
  assert(get_ctx().using_layout && thickness >= 0.f);
  
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
  assert(get_ctx().using_layout);

  auto& points = get_ctx().points;
  auto point_count = points.size();

  auto& vertices = get_ctx().vertices;
  auto& indices  = get_ctx().indices;
  uint32_t idx_beg    = vertices.size();
  uint32_t idx_offset = indices.size();

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

}}