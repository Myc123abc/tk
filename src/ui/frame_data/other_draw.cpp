#include "frame_data.hpp"

namespace tk::ui {

void FrameData::add_scissor_rect(Rect rect) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::add_scissor_rect);
  cmd.data.add_scissor_rect = { rect };
}

void FrameData::_add_scissor_rect(Rect rect) noexcept
{
  push_render_cmd(RenderCmdType::ui);

  for (auto idx : _render_cmd_rect_idxs)
    _render_cmds[idx].rect = rect;
  _render_cmd_rect_idxs.clear();
}

void FrameData::add_image(ImageHandle handle, float2 left_top, float2 right_bottom, uint8 alpha, std::span<float2> uvs) noexcept
{
  assert(uvs.size() == 4);
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::add_image);
  cmd.data.add_image = { handle, left_top, right_bottom, alpha, uvs[0], uvs[1], uvs[2], uvs[3] };
}

void FrameData::_add_image(ImageHandle handle, float2 left_top, float2 right_bottom, uint8 alpha, float2 uv0, float2 uv1, float2 uv2, float2 uv3) noexcept
{
  assert(!_using_discard_shapes && alpha);

  auto& img = renderer::g_img_mgr[handle];

  auto col    = Color{ 1, 1, 1, static_cast<float>(alpha) / 255 }.to_uint();
  auto packed = renderer::vtx_pack(renderer::VtxType::image, img.srv().index());
  auto [vertices, indices] = expand_beg(4, 6);
  vertices[0] = { left_top,                       uv0, col, packed };
  vertices[1] = { { right_bottom.x, left_top.y }, uv1, col, packed };
  vertices[2] = { right_bottom,                   uv2, col, packed };
  vertices[3] = { { left_top.x, right_bottom.y }, uv3, col, packed };
  indices[0]  = static_cast<uint16>(_vertex_beg + 0);
  indices[1]  = static_cast<uint16>(_vertex_beg + 1);
  indices[2]  = static_cast<uint16>(_vertex_beg + 2);
  indices[3]  = static_cast<uint16>(_vertex_beg + 0);
  indices[4]  = static_cast<uint16>(_vertex_beg + 2);
  indices[5]  = static_cast<uint16>(_vertex_beg + 3);
  expand_end();

  img.graphics_will_use();
}

void FrameData::add_text(TextParseResultHandle handle, float2 pos, float size, Color inner_color, Color outer_color, float outline_width) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::add_text);
  cmd.data.add_text = { handle, pos, size, inner_color, outer_color, outline_width };
}

void FrameData::_add_text(TextParseResultHandle handle, float2 pos, float size, Color inner_color, Color outer_color, float outline_width) noexcept
{
  auto const& result = g_text_engine.get_parse_result(handle);

  auto cnt = result.advances.size();

  auto [vertices, indices] = expand_beg(4 * cnt, 6 * cnt);
  auto vtx_offset = 0, idx_offset = 0;
  auto ascender   = result.is_vertical ? 0 : result.ascender;
  for (auto i = 0; i < cnt; ++i)
  {
    auto const& info = g_text_engine.get_glyph_info(result.glyph_info_keys[i]);
    auto scale = info.get_scale(size);

    info.set_vertices(vertices + vtx_offset, pos + result.offsets[i] * scale, size, ascender, inner_color, outer_color, outline_width);

    auto vtx_beg = _vertex_beg + vtx_offset;
    indices[idx_offset + 0] = static_cast<uint16>(vtx_beg + 0);
    indices[idx_offset + 1] = static_cast<uint16>(vtx_beg + 1);
    indices[idx_offset + 2] = static_cast<uint16>(vtx_beg + 2);
    indices[idx_offset + 3] = static_cast<uint16>(vtx_beg + 0);
    indices[idx_offset + 4] = static_cast<uint16>(vtx_beg + 2);
    indices[idx_offset + 5] = static_cast<uint16>(vtx_beg + 3);

    vtx_offset += 4;
    idx_offset += 6;
    pos        += result.advances[i] * scale;
  }
  expand_end();
}

}
