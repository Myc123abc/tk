#include "frame_data.hpp"
#include "../ui_context.hpp"

namespace tk::ui {

void FrameData::discard_beg(std::function<void()> func) noexcept
{
  _discard_beg_idx = _draw_cmds.size();
  _draw_cmds.emplace_back(DrawCmd::Type::discard_beg);
  auto beg = _draw_cmds.size();
  func();
  _draw_cmds[_discard_beg_idx].data.discard_beg.count = static_cast<uint>(_draw_cmds.size() - beg);
}

void FrameData::discard_end() noexcept
{
  // if not have any discard targets, remove the discard operation
  if (_draw_cmds[_discard_beg_idx].data.discard_beg.count + _discard_beg_idx == _draw_cmds.size() - 1)
    _draw_cmds.erase(_draw_cmds.begin() + _discard_beg_idx, _draw_cmds.end());
  else
    _draw_cmds.emplace_back(DrawCmd::Type::discard_end);
}

void FrameData::_discard_beg(uint count, uint& idx) noexcept
{
  assert(!_build_mode.contains(BuildMode::discard));
  assert(!_build_mode.contains(BuildMode::uni));
  _build_mode.add(BuildMode::discard);

  // push render cmd before this call
  // TODO: need be change because text render need outer_color and outline_width as render call constants
  if (_last_cmd_type) push_render_cmd(_last_cmd_type.value());

  _discard_vtx_beg      = _vertex_beg;
  _using_discard_shapes = true;
  for (auto i = 0u; i < count; ++i) build_render_cmd(_draw_cmds[++idx], idx);
  _using_discard_shapes = false;

  push_render_cmd_clear_rect(RenderCmdType::clear_discard_image, get_vertices_bound_rect(_discard_vtx_beg));
  push_render_cmd(RenderCmdType::discard_write);

  _clear_composite_image_cmd_idx = _render_cmds.size();
  push_render_cmd_clear_rect(RenderCmdType::clear_composite_image);

  _build_mode.add(BuildMode::discard);
  _discard_vtx_beg = _vertex_beg;
}

void FrameData::_discard_end() noexcept
{
  assert(_build_mode.contains(BuildMode::discard) && _vertices.size() > _discard_vtx_beg);

  _build_mode.remove(BuildMode::discard);

  auto rc = get_vertices_bound_rect(_discard_vtx_beg);
  _render_cmds[_clear_composite_image_cmd_idx].clear_rect = rc;
  push_render_cmd(RenderCmdType::discard_draw_composite);
  add_rect({ rc.left, rc.top }, { rc.right, rc.bottom });
  push_render_cmd(RenderCmdType::discard_composite);

  auto wnd = g_ui_ctx.window();
  _render_cmds.back().ui.scissor_rect = wnd->is_resizing() ? wnd->rect() : wnd->content_rect(); 
}

}
