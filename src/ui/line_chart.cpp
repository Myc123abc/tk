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

auto line_chart(float2 pos, LineChartInfo const& info) noexcept -> float2
{
  // get y layout
  auto y_layout = TextLayout(info.y_axis_tick_labels)
    .set_color(info.axis_color)
    .adjust_size(info.tick_label_size)
    .set_padding(info.y_tick_label_padding)
    .center_alignment(TextLayout::Direction::vertical, TextLayout::TextOrder::reverse);
  if (!info.origin_point_text.empty()) y_layout.ignore_text(0);

  // draw y label
  auto y_label_extent    = text(info.y_axis_label, info.label_size).extent;
  auto y_label_beg_point = float2{ pos.x, pos.y + (y_layout.height() + y_label_extent.x) / 2 };
  transform_beg(Transform::Rotate(y_label_beg_point, -90));
  text(info.y_axis_label, y_label_beg_point, info.label_size, info.axis_color);
  transform_end();

  // draw y tick labels
  y_layout.set_pos(pos.x + y_label_extent.y + info.y_label_padding, pos.y);
  g_ui_ctx.render(y_layout);

  // draw y axis
  auto y_axis_beg_point        = float2{ y_layout.pos().x + y_layout.width(), pos.y };
  auto y_axis_unit_height      = y_layout.unit_height();
  auto y_axis_half_unit_height = y_axis_unit_height / 2;
  auto origin_point            = float2{ y_axis_beg_point.x, pos.y + y_layout.height() - y_axis_half_unit_height };
  line(y_axis_beg_point, origin_point, info.axis_color);

  // draw x tick labels
  auto x_layout = TextLayout(info.x_axis_tick_labels)
    .set_color(info.axis_color)
    .adjust_size(info.tick_label_size)
    .set_padding(info.x_tick_label_padding);
  if (!info.origin_point_text.empty()) x_layout.ignore_text(0);
  auto x_axis_unit_width      = x_layout.unit_width();
  auto x_axis_half_unit_width = x_axis_unit_width / 2;
  x_layout.set_pos(origin_point.x - x_axis_half_unit_width, origin_point.y);
  g_ui_ctx.render(x_layout.center_alignment());

  // draw x axis
  auto x_axis_end_point = float2{ origin_point.x + x_layout.width() - x_axis_half_unit_width, origin_point.y };
  line(origin_point, x_axis_end_point, info.axis_color);

  // draw origin point
  if (!info.origin_point_text.empty())
  {
    auto ext = ui::text(info.origin_point_text, info.tick_label_size).extent;
    ui::text(info.origin_point_text, { origin_point.x - ext.x, origin_point.y }, info.tick_label_size, info.axis_color);
  }

  // draw x label
  auto x_label_extent = text(info.x_axis_label, info.label_size).extent;
  text(info.x_axis_label,
    { x_layout.pos().x + (x_layout.width() - x_label_extent.x) / 2, x_layout.pos().y + x_layout.extent().y  + info.x_label_padding},
    info.label_size, info.axis_color);

  // draw grid
  if (auto color = info.grid_color.value_or(0); color.a)
  {
    auto y = y_axis_beg_point.y + y_axis_half_unit_height;
    for (auto _ : std::views::iota(0u, y_layout.texts().size() - 1))
    {
      ui::line({ y_axis_beg_point.x, y }, { x_axis_end_point.x , y }, color);
      y += y_axis_unit_height;
    }

    auto x = origin_point.x + x_axis_unit_width;
    for (auto _ : std::views::iota(0u, x_layout.texts().size() - 1))
    {
      ui::line({ x, y_axis_beg_point.y }, { x , origin_point.y }, color);
      x += x_axis_unit_width;
    }
  }

  return {};
}

void test_line_chart() noexcept
{
  auto x_tick_values = std::vector<float>{ 0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
  auto y_tick_values = std::vector<float>{ 0, 10, 20, 30, 40, 50 };
  auto x_tick_labels = std::vector<std::string>{ "0", "1.0", "2.0", "3.0", "4.0", "5.0", "6.0" };
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

  line_chart({ 10, 10 }, info);
}

}
