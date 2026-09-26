#include "ui/line_chart.hpp"
#include "ui_context.hpp"

#include <ranges>

namespace tk::ui {

namespace {

auto get_ratios(std::span<float> nums) noexcept
{
  assert(nums.size() > 1);

  auto unit_interval_len = 0.f;

  // get intervals
  auto intervals = std::vector<float>{};
  intervals.reserve(nums.size() - 1);
  auto min = std::numeric_limits<float>::max();
  for (auto i : std::views::iota(0u, nums.size() - 1))
  {
    intervals.emplace_back(nums[i + 1] - nums[i]);
    min = std::min(min, intervals.back());
  }

  // get ratios
  auto ratios = std::vector<float>{};
  ratios.reserve(intervals.size());
  for (auto v : intervals)
  {
    ratios.emplace_back(v / min);
    if (!unit_interval_len && ratios.back() == 1)
      unit_interval_len = v;
  }

  return std::pair{ ratios, unit_interval_len };
}

}

auto split_x_y_values(std::span<float2> ps) noexcept -> std::pair<std::vector<float>, std::vector<float>>
{
  auto x_vs = std::vector<float>(ps.size());
  auto y_vs = std::vector<float>(ps.size());
  for (auto p : ps)
  {
    x_vs.emplace_back(p.x);
    y_vs.emplace_back(p.y);
  }
  return { x_vs, y_vs };
}

auto get_tick_values(std::span<float> vs, uint cnt) noexcept -> std::pair<std::vector<float>, uint>
{
  auto [min, max] = std::ranges::minmax(vs);
  auto range      = max - min;
  assert(cnt > 1 && range > 0);
  auto step       = range / (cnt - 1);

  auto magnitude = std::pow(10.f, std::floor(std::log10(step)));
  auto normalized = step / magnitude;
  if (normalized <= 1.f)  normalized = 1.f;
  else if (normalized <= 2.f) normalized = 2.f;
  else if (normalized <= 5.f) normalized = 5.f;
  else normalized = 10.f;
  step = normalized * magnitude;

  min = std::floor(min / step) * step;
  max = std::ceil(max / step) * step;
  cnt = std::round((max - min) / step) + 1;
  auto ticks = std::vector<float>(cnt);
  for (auto [i, tick] : std::views::enumerate(ticks))
    tick = min + static_cast<float>(i) * step;

  return { ticks, step };
}

auto get_tick_labels(std::span<float> vs, float step) noexcept -> std::vector<std::string>
{
  auto precision = step < 1.f ? static_cast<uint>(-std::floor(std::log10(step))) : 0;
  auto fmt       = std::format("{{:.{}f}}", precision);
  auto labels    = std::vector<std::string>(vs.size());
  for (auto [v, label] : std::views::zip(vs, labels))
    label = std::vformat(fmt, std::make_format_args(v));
  return labels;
}

void LineChart::calc_layout() noexcept
{
  auto text_cfg = TextConfig{};
  text_cfg.family = font_family;
  text_cfg.style  = font_style;

  auto [x_ratios, x_unit_interval_len] = get_ratios(x_axis_tick_values);
  auto [y_ratios, y_unit_interval_len] = get_ratios(y_axis_tick_values);

  _layout.x_unit_interval_len = x_unit_interval_len;
  _layout.y_unit_interval_len = y_unit_interval_len;

  // get y layout
  auto& y_layout = _layout.y_layout;
  y_layout = TextLayout(y_axis_tick_labels, font_family, font_style)
    .set_color(axis_color)
    .adjust_size(tick_label_size)
    .set_padding(y_tick_label_padding)
    .center_alignment(TextLayout::Direction::vertical, TextLayout::TextOrder::reverse)
    .adjust_ratios(y_ratios);
  if (!origin_point_text.empty()) y_layout.ignore_text(0);

  // get x layout
  auto& x_layout = _layout.x_layout;
  x_layout = TextLayout(x_axis_tick_labels, font_family, font_style)
    .set_color(axis_color)
    .adjust_size(tick_label_size)
    .set_padding(x_tick_label_padding)
    .center_alignment()
    .adjust_ratios(x_ratios);
  if (!origin_point_text.empty()) x_layout.ignore_text(0);

  // get x label info
  auto x_label_extent = text(x_axis_label, label_size, text_cfg).extent;
  _layout.x_label_width = x_label_extent.x;
  auto bounding = g_text_engine.get_bounding_rect(g_text_engine.parse(x_axis_label, {}, {}, {}));
  if (bounding)
    _layout.x_label_y_offset = -bounding->top * label_size / FT_Pixel_Size;

  // get extent
  auto y_label_extent = text(y_axis_label, label_size, text_cfg).extent;
  _layout.extent =
  {
    y_label_extent.y + y_layout.width() + x_layout.width() - x_layout.unit_width() / 2 + y_label_padding,
    y_layout.height() + x_layout.height() + x_label_extent.y - y_layout.unit_height() / 2 + x_label_padding + _layout.x_label_y_offset
  };
}

void LineChart::render(float2 pos) noexcept
{
  auto text_cfg = TextConfig{};
  text_cfg.family = font_family;
  text_cfg.style  = font_style;

  auto& x_layout = _layout.x_layout;
  auto& y_layout = _layout.y_layout;

  // draw y label
  auto y_label_extent    = text(y_axis_label, label_size, text_cfg).extent;
  auto y_label_beg_point = float2{ pos.x, pos.y + (y_layout.height() + y_label_extent.x) / 2 };
  transform_beg(Transform::Rotate(y_label_beg_point, -90));
  text(y_axis_label, y_label_beg_point, label_size, axis_color, text_cfg);
  transform_end();

  // get y axis layout info
  y_layout.set_pos(pos.x + y_label_extent.y + y_label_padding, pos.y);
  auto y_axis_beg_point        = float2{ y_layout.pos().x + y_layout.width(), pos.y };
  auto y_axis_unit_height      = y_layout.unit_height();
  auto y_axis_half_unit_height = y_axis_unit_height / 2;
  auto origin_point            = float2{ y_axis_beg_point.x, pos.y + y_layout.height() - y_axis_half_unit_height };

  // get x axis info
  auto x_axis_unit_width      = x_layout.unit_width();
  auto x_axis_half_unit_width = x_axis_unit_width / 2;
  auto x_axis_end_point       = float2{ origin_point.x + x_layout.width() - x_axis_half_unit_width, origin_point.y };
  x_layout.set_pos(origin_point.x - x_axis_half_unit_width, origin_point.y);
  
  // draw grid
  if (auto color = grid_color.value_or(0); color.a)
  {
    for (auto const& text : y_layout.texts() | std::views::drop(1))
    {
      auto const y = y_layout.pos().y + text.pos.y + text.extent.y / 2;
      line({ y_axis_beg_point.x, y }, { x_axis_end_point.x , y }, color);
    }

    for (auto const& text : x_layout.texts() | std::views::drop(1))
    {
      auto const x = x_layout.pos().x + text.pos.x + text.extent.x / 2;
      line({ x, y_axis_beg_point.y }, { x , origin_point.y }, color);
    }
  }

  // draw y tick labels
  g_ui_ctx.render(y_layout);

  // draw y axis
  line(y_axis_beg_point, origin_point, axis_color);

  // draw x tick labels
  g_ui_ctx.render(x_layout);

  // draw x axis
  line(origin_point, x_axis_end_point, axis_color);

  // draw origin point
  if (!origin_point_text.empty())
  {
    auto origin_text_width = text(origin_point_text, tick_label_size, text_cfg).extent.x;
    text(origin_point_text, { origin_point.x - origin_text_width, origin_point.y }, tick_label_size, axis_color, text_cfg);
  }

  // draw x tick labels
  text(x_axis_label,
    {
      x_layout.pos().x + (x_layout.width() - _layout.x_label_width) / 2,
      x_layout.pos().y + x_layout.extent().y  + x_label_padding + _layout.x_label_y_offset
    },
    label_size, axis_color, text_cfg);

  // draw datas
  auto get_offset_ratio = [](float v, std::span<float> vs, float unit_interval_len)
  {
    assert(vs.size() > 1);
    auto it = std::ranges::lower_bound(vs, v);
    if (it != vs.end())
    {
      if (it == vs.begin())
        return -(vs[0] - v) / unit_interval_len;
      return (v - vs[0]) / unit_interval_len;
    }
    else
      return (v - vs[0]) / unit_interval_len;
  };

  auto get_x = [&](float v)
  {
    auto ratio = get_offset_ratio(v, x_axis_tick_values, _layout.x_unit_interval_len);
    return origin_point.x + x_layout.unit_width() * ratio; 
  };

  auto get_y = [&](float v)
  {
    auto ratio = get_offset_ratio(v, y_axis_tick_values, _layout.y_unit_interval_len);
    return origin_point.y - y_layout.unit_height() * ratio; 
  };

  for (auto const& [points, color] : datas)
  {
    for (auto [a, b] : points | std::views::pairwise)
    {
      auto p0 = float2{ get_x(a.x), get_y(a.y) };
      auto p1 = float2{ get_x(b.x), get_y(b.y) };
      line(p0, p1, color);
    }
  }
}

}
