#include "frame_data.hpp"
#include "util.hpp"
#include "../ui_context.hpp"

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

void FrameData::transform_beg(Matrix const& transform) noexcept
{
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::transform_beg);
  cmd.data.transform_beg = { transform };
}

void FrameData::_transform_beg(Matrix const& transform) noexcept
{
  if (_transform_stack.empty())
    _transform_stack.emplace_back(transform);
  else
    _transform_stack.emplace_back(_transform_stack.back() * transform);
}

void FrameData::transform_end() noexcept
{
  _draw_cmds.emplace_back(DrawCmd::Type::transform_end);
}

void FrameData::_transform_end() noexcept
{
  assert(!_transform_stack.empty());
  _transform_stack.pop_back();
}

void FrameData::transform_vertices(uint vtx_beg) noexcept
{
  if (_transform_stack.empty())
    return;

  auto const& transform = _transform_stack.back();
  for (auto i = vtx_beg; i < _vertex_beg; ++i)
    _vertices[i].pos = transform * _vertices[i].pos;
}

auto FrameData::transform_point(float2 p) const noexcept -> float2
{
  if (_transform_stack.empty())
    return p;
  return _transform_stack.back() * p;
}

void FrameData::add_image(ImageHandle handle, float2 p0, float2 p1, float2 p2, float2 p3, uint8 alpha, std::span<float2> uvs) noexcept
{
  assert(uvs.size() == 4);
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::add_image);
  cmd.data.add_image = { handle, p0, p1, p2, p3, alpha, uvs[0], uvs[1], uvs[2], uvs[3] };
}

void FrameData::add_image(ImageHandle handle, float2 left_top, float2 right_bottom, uint8 alpha, std::span<float2> uvs) noexcept
{
  add_image(handle, left_top, { right_bottom.x, left_top.y }, right_bottom, { left_top.x, right_bottom.y }, alpha, uvs);
}

void FrameData::add_image_rounded(ImageHandle handle, float2 left_top, float2 right_bottom, uint8 alpha, std::span<float2> uvs, float rounding, Flag<CornerFlag> flags) noexcept
{
  assert(uvs.size() == 4);
  auto& cmd = _draw_cmds.emplace_back(DrawCmd::Type::add_image_rounded);
  cmd.data.add_image_rounded = { handle, left_top, right_bottom, alpha, uvs[0], uvs[1], uvs[2], uvs[3], rounding, flags };
}

void FrameData::_add_image(ImageHandle handle, float2 p0, float2 p1, float2 p2, float2 p3, uint8 alpha, float2 uv0, float2 uv1, float2 uv2, float2 uv3) noexcept
{
  assert(!_using_discard_shapes && alpha);

  auto& img = renderer::g_img_mgr[handle];

  auto col    = Color{ 1, 1, 1, static_cast<float>(alpha) / 255 }.to_uint();
  auto packed = renderer::vtx_pack(renderer::VtxType::image, img.srv().index());
  auto [vertices, indices] = expand_beg(4, 6);
  vertices[0] = { p0, uv0, col, packed };
  vertices[1] = { p1, uv1, col, packed };
  vertices[2] = { p2, uv2, col, packed };
  vertices[3] = { p3, uv3, col, packed };
  indices[0]  = static_cast<uint16>(_vertex_beg + 0);
  indices[1]  = static_cast<uint16>(_vertex_beg + 1);
  indices[2]  = static_cast<uint16>(_vertex_beg + 2);
  indices[3]  = static_cast<uint16>(_vertex_beg + 0);
  indices[4]  = static_cast<uint16>(_vertex_beg + 2);
  indices[5]  = static_cast<uint16>(_vertex_beg + 3);
  expand_end();

  _sampled_images.emplace(handle);
}

void FrameData::_add_image_rounded(ImageHandle handle, float2 left_top, float2 right_bottom, uint8 alpha, float2 uv0, float2 uv1, float2 uv2, float2 uv3, float rounding, Flag<CornerFlag> flags) noexcept
{
  assert(!_using_discard_shapes && alpha);

  assert(_points.empty());
  path_rect(left_top, right_bottom, rounding, flags);
  auto const pt_cnt = static_cast<uint>(_points.size());
  if (pt_cnt == 4)
  {
    _points.clear();
    _add_image(handle, left_top, { right_bottom.x, left_top.y }, right_bottom, { left_top.x, right_bottom.y }, alpha, uv0, uv1, uv2, uv3);
    return;
  }

  auto& img = renderer::g_img_mgr[handle];

  auto const aa_width = g_ui_ctx.window()->scale();
  auto const col      = Color{ 1, 1, 1, static_cast<float>(alpha) / 255 }.to_uint();
  auto const aa_col   = Color{ 1, 1, 1, 0.f }.to_uint();
  auto const packed   = renderer::vtx_pack(renderer::VtxType::image, img.srv().index());
  auto [vertices, indices] = expand_beg(pt_cnt * 2, (pt_cnt - 2) * 3 + pt_cnt * 6);

  auto calc_uv = [=] (float2 p) noexcept
  {
    auto const ext = right_bottom - left_top;
    auto const t   = (p - left_top) / ext;
    return float2{
      uv0.x + (uv2.x - uv0.x) * t.x,
      uv0.y + (uv2.y - uv0.y) * t.y,
    };
  };

  auto inner_idx = _vertex_beg;
  auto outer_idx = _vertex_beg + 1;
  for (auto i = 2; i < pt_cnt; ++i)
  {
    indices[0] = static_cast<uint16>(inner_idx);
    indices[1] = static_cast<uint16>(inner_idx + ((i - 1) << 1));
    indices[2] = static_cast<uint16>(inner_idx + (i << 1));
    indices += 3;
  }

  _normals.resize(pt_cnt);
  for (uint i0 = pt_cnt - 1, i1 = 0; i1 < pt_cnt; i0 = i1++)
  {
    auto dp = fast_normalize(_points[i1] - _points[i0]);
    _normals[i0] = { dp.y, -dp.x };
  }

  for (uint i0 = pt_cnt - 1, i1 = 0; i1 < pt_cnt; i0 = i1++)
  {
    auto dmp = fix_normal((_normals[i0] + _normals[i1]) * .5f) * aa_width * .5f;
    auto inner_pos = _points[i1] - dmp;
    auto outer_pos = _points[i1] + dmp;
    vertices[0] = { inner_pos, calc_uv(inner_pos), col,    packed };
    vertices[1] = { outer_pos, calc_uv(outer_pos), aa_col, packed };
    vertices += 2;

    indices[0] = static_cast<uint16>(inner_idx + (i1 << 1));
    indices[1] = static_cast<uint16>(inner_idx + (i0 << 1));
    indices[2] = static_cast<uint16>(outer_idx + (i0 << 1));
    indices[3] = static_cast<uint16>(outer_idx + (i0 << 1));
    indices[4] = static_cast<uint16>(outer_idx + (i1 << 1));
    indices[5] = static_cast<uint16>(inner_idx + (i1 << 1));
    indices += 6;
  }

  expand_end();
  _points.clear();

  _sampled_images.emplace(handle);
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

    _sampled_images.emplace(g_text_engine.glyph_atlas(info.glyph_atlas_index));

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
