#include "ui/line_chart.hpp"
#include "text_engine/text_layout.hpp"
#include "ui_context.hpp"

namespace tk::ui {

struct LineChartInfo
{
  std::string_view             x_axis_label;
  std::string_view             y_axis_label;
  std::span<float>             x_axis_tick_values;
  std::span<float>             y_axis_tick_values;
  std::span<std::string const> x_axis_tick_labels;
  std::span<std::string const> y_axis_tick_labels;
  float2                       x_tick_label_padding;
  float2                       y_tick_label_padding;
  Color                        axis_color;
  float                        tick_label_size;
  std::optional<Color>         grid_color;
  float                        label_size;
  float                        x_label_padding;
  float                        y_label_padding;
  std::string_view             origin_point_text;

  struct Data
  {
    std::span<float2> points;
    Color             color;
  };
  std::span<Data> datas;
};

auto get_intervals_ratios(std::span<float> nums) noexcept -> std::pair<std::vector<float>, std::vector<float>>
{
  assert(nums.size() > 1);

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
    ratios.emplace_back(v / min);

  return { intervals, ratios };
}

struct LineChartLayout
{
  float2     y_label_beg_point;
  TextLayout y_layout;
  float2     y_axis_beg_point;
  float2     origin_point;
  TextLayout x_layout;
  float2     x_axis_end_point;
  float      origin_text_width{};
  float      x_label_width{};
  float2     extent;
};

void render_line_chart(LineChartInfo const& info, LineChartLayout const& layout) noexcept
{
  // draw y label
  transform_beg(Transform::Rotate(layout.y_label_beg_point, -90));
  text(info.y_axis_label, layout.y_label_beg_point, info.label_size, info.axis_color);
  transform_end();

  // draw y tick labels
  g_ui_ctx.render(layout.y_layout);

  // draw y axis
  line(layout.y_axis_beg_point, layout.origin_point, info.axis_color);

  // draw x tick labels
  g_ui_ctx.render(layout.x_layout);

  // draw x axis
  line(layout.origin_point, layout.x_axis_end_point, info.axis_color);

  // draw origin point
  if (!layout.origin_text_width)
    ui::text(info.origin_point_text, { layout.origin_point.x - layout.origin_text_width, layout.origin_point.y }, info.tick_label_size, info.axis_color);

  // draw x tick labels
  text(info.x_axis_label,
    { layout.x_layout.pos().x + (layout.x_layout.width() - layout.x_label_width) / 2, layout.x_layout.pos().y + layout.x_layout.extent().y  + info.x_label_padding},
    info.label_size, info.axis_color);
  
  // draw grid
  if (auto color = info.grid_color.value_or(0); color.a)
  {
    for (auto const& text : layout.y_layout.texts() | std::views::drop(1))
    {
      auto const y = layout.y_layout.pos().y + text.pos.y + text.extent.y / 2;
      ui::line({ layout.y_axis_beg_point.x, y }, { layout.x_axis_end_point.x , y }, color);
    }

    for (auto const& text : layout.x_layout.texts() | std::views::drop(1))
    {
      auto const x = layout.x_layout.pos().x + text.pos.x + text.extent.x / 2;
      ui::line({ x, layout.y_axis_beg_point.y }, { x , layout.origin_point.y }, color);
    }
  }
}

auto line_chart_layout(float2 pos, LineChartInfo const& info) noexcept -> LineChartLayout
{
  auto layout = LineChartLayout{};

  auto [x_intervals, x_ratios] = get_intervals_ratios(info.x_axis_tick_values);
  auto [y_intervals, y_ratios] = get_intervals_ratios(info.y_axis_tick_values);

  // get y layout
  auto& y_layout = layout.y_layout;
  y_layout = TextLayout(info.y_axis_tick_labels)
    .set_color(info.axis_color)
    .adjust_size(info.tick_label_size)
    .set_padding(info.y_tick_label_padding)
    .center_alignment(TextLayout::Direction::vertical, TextLayout::TextOrder::reverse)
    .adjust_ratios(y_ratios);
  if (!info.origin_point_text.empty()) y_layout.ignore_text(0);

  // get y label info
  auto y_label_extent = text(info.y_axis_label, info.label_size).extent;
  layout.y_label_beg_point = { pos.x, pos.y + (y_layout.height() + y_label_extent.x) / 2 };

  // set position of y tick labels
  y_layout.set_pos(pos.x + y_label_extent.y + info.y_label_padding, pos.y);

  // get y axis layout info
  auto y_axis_beg_point        = float2{ y_layout.pos().x + y_layout.width(), pos.y };
  auto y_axis_unit_height      = y_layout.unit_height();
  auto y_axis_half_unit_height = y_axis_unit_height / 2;
  auto origin_point            = float2{ y_axis_beg_point.x, pos.y + y_layout.height() - y_axis_half_unit_height };
  layout.y_axis_beg_point = y_axis_beg_point;
  layout.origin_point     = origin_point;

  // get x layout
  auto& x_layout = layout.x_layout;
  x_layout = TextLayout(info.x_axis_tick_labels)
    .set_color(info.axis_color)
    .adjust_size(info.tick_label_size)
    .set_padding(info.x_tick_label_padding);
  if (!info.origin_point_text.empty()) x_layout.ignore_text(0);
  auto x_axis_unit_width      = x_layout.unit_width();
  auto x_axis_half_unit_width = x_axis_unit_width / 2;
  x_layout
    .set_pos(origin_point.x - x_axis_half_unit_width, origin_point.y)
    .center_alignment()
    .adjust_ratios(x_ratios);

  // get x axis info
  auto x_axis_end_point = float2{ origin_point.x + x_layout.width() - x_axis_half_unit_width, origin_point.y };
  layout.x_axis_end_point = x_axis_end_point;

  // get origin point info
  if (!info.origin_point_text.empty())
    layout.origin_text_width = ui::text(info.origin_point_text, info.tick_label_size).extent.x;

  // get x label info
  auto x_label_extent = text(info.x_axis_label, info.label_size).extent;
  layout.x_label_width = x_label_extent.x;

  layout.extent = { x_axis_end_point.x - pos.x, x_axis_end_point.y + x_layout.height() + x_label_extent.y - pos.y };

  return layout;
}

void test_line_chart() noexcept
{
  // TODO: data lower or bigger than y ticks
  // auto x_tick_values = std::vector<float>{ 0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
  auto x_tick_values = std::vector<float>{ 0, 1.0, 3.0, 4.0, 5.0, 6.0 };
  auto y_tick_values = std::vector<float>{ 0, 10, 20, 30, 40, 50 };
  // auto x_tick_labels = std::vector<std::string>{ "0", "1.0", "2.0", "3.0", "4.0", "5.0", "6.0" };
  auto x_tick_labels = std::vector<std::string>{ "0", "1.0", "3.0", "4.0", "5.0", "6.0" };
  auto y_tick_labels = std::vector<std::string>{ "0", "10", "20", "30", "40", "50" };
  auto data1 = std::vector<float2>
  {
    { 0, 0 },
    { 1, 3 },
    { 2, 7 },
    { 3, 12 },
    { 4, 18 },
    { 5, 30 },
    { 6, 45.6 },
  };
  auto data = std::vector{ LineChartInfo::Data{ data1, 0x0000ffff } };

  auto info = LineChartInfo{};
  info.x_axis_label = "Time (s)";
  info.y_axis_label = "Speed (m\\s)";
  info.x_axis_tick_values = x_tick_values;
  info.y_axis_tick_values = y_tick_values;
  info.x_axis_tick_labels = x_tick_labels;
  info.y_axis_tick_labels = y_tick_labels;
  info.x_tick_label_padding = { 32, 8 };
  info.y_tick_label_padding = { 8, 32 };
  info.datas = data;
  info.axis_color = 0xffffffff;
  info.tick_label_size = 12;
  info.grid_color = 0x404040ff;
  info.label_size = 14;
  info.y_label_padding = 4;
  info.origin_point_text = "0";

  auto pos    = float2(10);
  auto layout = line_chart_layout(pos, info);
  render_line_chart(info, layout);
  ui::rectangle(pos, pos + layout.extent, 0x00ff00ff, 1);
}

}
