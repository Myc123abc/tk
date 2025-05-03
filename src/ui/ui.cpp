#include "tk/ui/ui.hpp"
#include "internal.hpp"

#include <cassert>

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

}}