#include "tk/benchmark.hpp"

#include "tk/tk.hpp"
#include "tk/error_handling.hpp"
#include "tk/ui/line_chart.hpp"

#include <ranges>

using namespace tk;

namespace tk::benchmark {

auto g_wndCfg     = ui::WindowConfig{};
auto g_is_closed  = false;
auto g_x_label    = StringLiteral{};
auto g_y_label    = StringLiteral{};
auto g_limit_size = float2{};

struct LineChartInfo
{
  struct Data
  {
    std::span<float> xs;
    std::span<float> ys;
    uint32_t         color;
    StringLiteral    legend;
    Rect             text_rect;
  };

  ui::LineChart                    line_chart;
  std::vector<Data>                datas;
  std::vector<ui::LineChart::Data> line_chart_datas;
  std::vector<std::vector<float2>> points;
  std::vector<float>               x_tick_values;
  std::vector<float>               y_tick_values;

  void set_datas() noexcept;
  void render(float2 pos) noexcept;
};

auto g_line_chart_infos = std::vector<LineChartInfo>{};

void init_line_chart(ui::LineChart& line_chart) noexcept
{
  line_chart.x_tick_label_padding = { 32, 8 };
  line_chart.y_tick_label_padding = { 16, 32 };
  line_chart.axis_color           = 0xffffffff;
  line_chart.tick_label_size      = 12;
  line_chart.grid_color           = 0x404040ff;
  line_chart.label_size           = 14;
  line_chart.y_label_padding      = 4;
}

void init_line_chart_info() noexcept
{
  init_line_chart(g_line_chart_infos.emplace_back(ui::LineChart{}).line_chart);
}

void init() noexcept
{
  tk::init();

  err_if(!ui::load_font("C:/Windows/Fonts/segoeui.ttf"), "failed to load font");

  g_wndCfg.display_title_bar             = true;
  g_wndCfg.display_window_shadow         = true;
  g_wndCfg.display_wireframe_only_active = true;
  g_wndCfg.wireframe_color               = 0x0000ffff;

  init_line_chart_info();
}

void destroy() noexcept
{
  tk::destroy();
}

auto running() noexcept -> bool
{
  ui::begin("Benchmark", 0, 0, 200, 200, &g_is_closed, g_wndCfg);
  ui::rectangle({}, ui::window_drawable_extent(), 0x000000ff);
  return !g_is_closed;
}

void set_labels(StringLiteral x_label, StringLiteral y_label) noexcept
{
  g_x_label = x_label;
  g_y_label = y_label;
}

void limit_size(float width, float height) noexcept
{
  g_limit_size = { width, height };
}

void present(std::span<float> xs, std::span<float> ys, uint32_t color, StringLiteral legend) noexcept
{
  assert(xs.size() == ys.size());
  g_line_chart_infos.back().datas.emplace_back(xs, ys, color, legend);
}

auto to_all_vs = [](auto&& range, auto&& mem)
{
  return range | std::views::transform(mem) | std::views::join | std::ranges::to<std::vector<float>>();
};

void LineChartInfo::set_datas() noexcept
{
  auto xs = to_all_vs(datas, &LineChartInfo::Data::xs);
  auto ys = to_all_vs(datas, &LineChartInfo::Data::ys);
 
  auto [x_vs, x_step] = ui::get_tick_values(xs);
  auto [y_vs, y_step] = ui::get_tick_values(ys);
  auto x_ts = ui::get_tick_labels(x_vs, x_step);
  auto y_ts = ui::get_tick_labels(y_vs, y_step);
 
  x_tick_values = std::move(x_vs);
  y_tick_values = std::move(y_vs);

  line_chart.x_axis_label       = g_x_label;
  line_chart.y_axis_label       = g_y_label;
  line_chart.x_axis_tick_values = x_tick_values;
  line_chart.y_axis_tick_values = y_tick_values;
  line_chart.x_axis_tick_labels = x_ts;
  line_chart.y_axis_tick_labels = y_ts;
 
  // get draw information
  line_chart_datas.reserve(datas.size());
  points.reserve(datas.size());
  for (auto const& data : datas)
  {
    points.emplace_back(std::views::zip_transform([](auto x, auto y) { return float2{ x, y }; }, data.xs, data.ys)
      | std::ranges::to<std::vector<float2>>());
    line_chart_datas.emplace_back(points.back(), data.color);
  }
  line_chart.datas = line_chart_datas;
 
  // draw line chart
  auto advice_counts = line_chart.calc_layout(g_limit_size);
  if (advice_counts.x)
  {
    ui::adjust_tick_values(x_tick_values, xs, advice_counts.x);
    x_ts = ui::get_tick_labels(x_tick_values, x_step);
    line_chart.x_axis_tick_values = x_tick_values;
    line_chart.x_axis_tick_labels = x_ts;
  }
  if (advice_counts.y)
  {
    ui::adjust_tick_values(y_tick_values, ys, advice_counts.y);
    y_ts = ui::get_tick_labels(y_tick_values, y_step);
    line_chart.y_axis_tick_values = y_tick_values;
    line_chart.y_axis_tick_labels = y_ts;
  }
  if (advice_counts.x || advice_counts.y) line_chart.calc_layout();
}

void LineChartInfo::render(float2 pos) noexcept
{
  line_chart.render(pos);
 
  // get legends max height
  auto max_height = 0.f;
  auto max_width  = 0.f;
  for (auto& [xs, ys, color, legend, text_rect] : datas)
  {
    if (auto rc = ui::get_text_bounding_rect(legend, line_chart.label_size))
    {
      text_rect  = rc.value();
      max_height = std::max(max_height, text_rect.height());
      max_width  = std::max(max_width, text_rect.width());
    }
  }

  // draw legends
  auto line_chart_ext = line_chart.extent();
  auto padding        = 5;
  auto beg_pos        = float2{ pos.x + line_chart_ext.x + 10, pos.y };
  auto extent         = float2{ padding + max_height + padding + max_width + padding, padding };
  auto p              = beg_pos + float2(padding);
  for (auto& [xs, ys, color, legend, text_rect] : datas)
  {
    if (!text_rect.empty())
    {
      ui::rectangle(p, p + float2(max_height), color);
      auto offset = (max_height - text_rect.height()) / 2;
      ui::text(legend, p + float2{ max_height + padding, offset - text_rect.top }, line_chart.label_size, line_chart.axis_color);
      offset    = max_height + padding;
      p.y      += offset;
      extent.y += offset;
    }
  }
  // draw legends border
  ui::rectangle(beg_pos, beg_pos + extent, line_chart.axis_color, 1);
}

void update() noexcept
{
  auto pos = float2(10);

  // set last data for line chart
  g_line_chart_infos.back().set_datas();

  // render line charts
  auto max_origin_point_x = 0.f;
  for (auto const& info : g_line_chart_infos)
    max_origin_point_x = std::max(max_origin_point_x, info.line_chart.origin_point().x);
  for (auto& info : g_line_chart_infos)
  {
    auto offset = max_origin_point_x - info.line_chart.origin_point().x;
    info.render({ pos.x + offset, pos.y });
    pos.y += info.line_chart.extent().y + 10;
  }

  ui::end();
  tk::update();

  g_line_chart_infos.clear();
  init_line_chart_info();
}

void new_line_chart() noexcept
{
  g_line_chart_infos.back().set_datas();
  init_line_chart_info();
}

}
