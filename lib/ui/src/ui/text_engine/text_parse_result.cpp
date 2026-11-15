#include "text_parse_result.hpp"
#include "../../util/unicode.hpp"

extern "C"
{
#include <linebreak.h>
}

namespace tk::ui {

void TextParseResult::limit_length(std::string_view text, float len) noexcept
{
  if (len <= 0.f) return;

  auto cluster_end = [&](uint beg)
  {
    auto end = beg + 1;
    while (end < clusters.size() && clusters[end] == clusters[beg]) ++end;
    return end;
  };
  auto cluster_length = [&](uint beg, uint end)
  {
    auto advance = 0.f;
    if (direction == TextDirection::horizontal)
    {
      for (auto i = beg; i < end; ++i)
        advance += advances[i].x;
    }
    else
    {
      for (auto i = beg; i < end; ++i)
        advance += advances[i].y;
    }
    return std::abs(advance);
  };

  auto max_cluster_length = 0.f;
  for (auto beg = 0u; beg < advances.size();)
  {
    auto const end = cluster_end(beg);
    max_cluster_length = std::max(max_cluster_length, cluster_length(beg, end));
    beg = end;
  }
  len = std::max(len, max_cluster_length);

  auto line_breaks = std::vector<char>(text.size());
  set_linebreaks_utf8(reinterpret_cast<utf8_t const*>(text.data()), text.size(), nullptr, line_breaks.data());

  auto can_break_after = [&](uint end)
  {
    auto const text_end = end < clusters.size() ? clusters[end] : static_cast<uint>(text.size());
    if (!text_end) return false;
    auto const line_break = line_breaks[text_end - 1];
    return line_break == LINEBREAK_ALLOWBREAK || line_break == LINEBREAK_MUSTBREAK;
  };

  auto line_count         = 1u;
  auto line_has_cluster   = false;
  auto line_length        = 0.f;
  auto max_line_length    = 0.f;
  auto pen                = float2{};
  auto previous_glyph_pos = float2{};
  auto line_begin         = 0u;
  auto break_end          = 0u;
  auto break_length       = 0.f;
  auto break_glyph_pos    = float2{};

  for (auto beg = 0u; beg < advances.size();)
  {
    auto const end    = cluster_end(beg);
    auto const length = cluster_length(beg, end);

    if (line_has_cluster && line_length + length > len)
    {
      auto const wrap_begin        = break_end > line_begin ? break_end       : beg;
      auto const wrapped_length    = break_end > line_begin ? break_length    : line_length;
      auto const wrapped_glyph_pos = break_end > line_begin ? break_glyph_pos : previous_glyph_pos;

      max_line_length = std::max(max_line_length, wrapped_length);
      ++line_count;
      auto next_line_pos = direction == TextDirection::horizontal
        ? float2{ 0.f, static_cast<float>(line_count - 1) * extent.y }
        : float2{ -static_cast<float>(line_count - 1) * extent.x, 0.f };

      advances[wrap_begin - 1] = next_line_pos - wrapped_glyph_pos;
      pen          = next_line_pos;
      line_length -= wrapped_length;
      line_begin   = wrap_begin;
      break_end    = {};

      for (auto i = wrap_begin; i < beg; ++i)
      {
        previous_glyph_pos = pen;
        pen               += advances[i];
      }
    }

    line_length      += length;
    line_has_cluster = true;
    for (auto i = beg; i < end; ++i)
    {
      previous_glyph_pos = pen;
      pen               += advances[i];
    }
    if (can_break_after(end))
    {
      break_end       = end;
      break_length    = line_length;
      break_glyph_pos = previous_glyph_pos;
    }
    beg = end;
  }

  max_line_length = std::max(max_line_length, line_length);
  if (direction == TextDirection::horizontal)
    extent = { max_line_length, static_cast<float>(line_count) * extent.y };
  else
    extent = { static_cast<float>(line_count) * extent.x, max_line_length };
}

void TextParseResult::apply_tracking(float tracking) noexcept
{
  if (tracking == 0.f || advances.empty()) return;

  auto cluster_count = 0u;
  for (auto begin = 0u; begin < advances.size();)
  {
    auto end = begin + 1;
    while (end < clusters.size() && clusters[end] == clusters[begin]) ++end;
    if (end < advances.size())
    {
      if (direction == TextDirection::horizontal)
        advances[end - 1].x += tracking;
      else
        advances[end - 1].y += tracking;
      ++cluster_count;
    }
    begin = end;
  }

  if (direction == TextDirection::horizontal)
    extent.x += tracking * cluster_count;
  else
    extent.y += tracking * cluster_count;
}

auto glyph_rect(GlyphInfo const& info, float2 pos, bool sideways, bool visible) noexcept -> Rect
{
  auto const offset = info.pos_offset + (visible ? info.visible_offset : float2{});
  auto const extent = visible ? info.visible_extent : info.extent;
  if (!sideways)
    return { pos + offset, pos + offset + extent };
  return
  {
    pos + float2{ -offset.y - extent.y, offset.x            },
    pos + float2{ -offset.y,            offset.x + extent.x },
  };
}

void TextParseResult::prepare_layout(GlyphKeyMap<GlyphInfo> const& glyph_infos) noexcept
{
  assert(generate_complete());

  if (direction != TextDirection::vertical)
    return;

  // Work in cluster-local coordinates. Tracking and column wrapping are added
  // later, after visible color-glyph ink has repaired undersized advances.
  auto const cell_height = ascender * 2.f;
  auto column_left       = std::numeric_limits<float>::max();
  auto column_right      = std::numeric_limits<float>::lowest();
  for (auto begin = 0u; begin < glyph_info_keys.size();)
  {
    auto end = begin + 1;
    while (end < clusters.size() && clusters[end] == clusters[begin])
      ++end;

    auto has_visible_glyph = false;
    auto has_color_glyph   = false;
    auto ink_left          = std::numeric_limits<float>::max();
    auto ink_right         = std::numeric_limits<float>::lowest();
    auto render_left       = std::numeric_limits<float>::max();
    auto render_right      = std::numeric_limits<float>::lowest();
    auto ink_top           = std::numeric_limits<float>::max();
    auto ink_bottom        = std::numeric_limits<float>::lowest();
    auto pen_y             = 0.f;
    auto cluster_advance   = 0.f;
    for (auto i = begin; i < end; ++i)
    {
      auto const& info          = glyph_infos.at(glyph_info_keys[i]);
      auto const  sideways      = is_sideways(orientations[i]);
      auto const  glyph_pos     = float2{ 0.f, pen_y } + offsets[i];
      auto const  render_bounds = glyph_rect(info, glyph_pos, sideways, false);
      render_left  = std::min(render_left, render_bounds.left);
      render_right = std::max(render_right, render_bounds.right);
      if (info.visible_extent.x > 0.f && info.visible_extent.y > 0.f)
      {
        auto const bounds = glyph_rect(info, glyph_pos, sideways, true);
        ink_left          = std::min(ink_left, bounds.left);
        ink_right         = std::max(ink_right, bounds.right);
        if (!sideways)
        {
          ink_top    = std::min(ink_top, bounds.top);
          ink_bottom = std::max(ink_bottom, bounds.bottom);
        }
        has_visible_glyph = true;
        has_color_glyph  |= info.is_color;
      }
      if (i + 1 < end)
        pen_y += advances[i].y;
      cluster_advance += advances[i].y;
    }

    if (has_visible_glyph)
    {
      auto const orientation = orientations[begin];
      auto const composed    = orientation == GlyphOrientation::upright_composed;
      if (composed || has_color_glyph || orientation == GlyphOrientation::sideways_centered)
      {
        auto const offset = cell_height * .5f - (ink_left + ink_right) * .5f;
        for (auto i = begin; i < end; ++i)
          offsets[i].x += offset;
        render_left  += offset;
        render_right += offset;
      }
      column_left  = std::min(column_left, render_left);
      column_right = std::max(column_right, render_right);
      if ((composed || has_color_glyph) && ink_top <= ink_bottom)
      {
        auto const offset = (cell_height - (ink_bottom - ink_top)) * .5f - ink_top;
        for (auto i = begin; i < end; ++i)
          offsets[i].y += offset;
        ink_bottom += offset;
      }
      if (has_color_glyph && !is_sideways(orientation))
      {
        auto const minimum_advance = std::max(0.f, ink_bottom);
        if (cluster_advance < minimum_advance)
        {
          auto const adjustment = minimum_advance - cluster_advance;
          advances[end - 1].y += adjustment;
          extent.y            += adjustment;
        }
      }
    }
    begin = end;
  }

  auto const column_width = std::max(cell_height, column_right - column_left);
  auto const column_scale = column_width / cell_height;
  if (column_scale > 1.f)
  {
    for (auto& advance : advances)
      advance.x *= column_scale;
    extent.x *= column_scale;
  }
}

void TextParseResult::finalize(GlyphKeyMap<GlyphInfo> const& glyph_infos) noexcept
{
  assert(generate_complete());

  auto needs_ink_bounds = false;
  auto rect             = Rect{};
  auto pos              = float2{};
  auto baseline         = direction == TextDirection::vertical ? 0.f : ascender;
  for (auto i = 0u; i < glyph_info_keys.size(); ++i)
  {
    auto const& info     = glyph_infos.at(glyph_info_keys[i]);
    auto const  sideways = is_sideways(orientations[i]);
    needs_ink_bounds |= info.is_color || sideways;
    auto const bounds = glyph_rect(info, pos + offsets[i] + float2{ 0.f, baseline }, sideways, false);
    rect.expand(bounds.left_top());
    rect.expand(bounds.right_bottom());
    pos += advances[i];
  }

  if (!needs_ink_bounds || rect.empty()) return;

  auto line_count = 1u;
  if (direction == TextDirection::horizontal)
    line_count += std::ranges::count_if(advances, [](auto advance) { return advance.y != 0.f; });
  else
    line_count += std::ranges::count_if(advances, [](auto advance) { return advance.x != 0.f; });

  auto layout_rect = Rect{};
  if (direction == TextDirection::horizontal)
    layout_rect = { float2{}, extent };
  else
  {
    auto const column_width = extent.x / line_count;
    layout_rect = { -static_cast<float>(line_count - 1) * column_width, 0.f, column_width, extent.y };
  }
  rect.expand(layout_rect.left_top());
  rect.expand(layout_rect.right_bottom());

  layout_offset = -rect.left_top();
  extent        = rect.extent();
  if (direction == TextDirection::horizontal)
  {
    ascender       += layout_offset.y;
    layout_offset.y = 0.f;
  }
  bounding_rect = Rect{ float2{}, extent };
}

TextParseResult::TextParseResult(std::string_view text, TextDirection direction) noexcept
{
  auto size = codepoint_cnt(text);
  advances.reserve(size);
  offsets.reserve(size);
  clusters.reserve(size);
  orientations.reserve(size);
  glyph_info_keys.reserve(size);
  this->direction = direction;
}

}
