#include "tk/benchmark.hpp"

#include "tk/tk.hpp"
#include "tk/error_handling.hpp"
#include "tk/ui/line_chart.hpp"

#include <ranges>

using namespace tk;

namespace tk::benchmark {

auto g_wndCfg     = ui::WindowConfig{};
auto g_line_chart = ui::LineChart{};
auto g_is_closed  = false;
auto g_x_label    = StringLiteral{};
auto g_y_label    = StringLiteral{};
auto g_pos        = float2(10);
auto g_limit_size = float2{};

struct LineChartData
{
  struct Points
  {
    std::span<float> xs;
    std::span<float> ys;
    uint32_t         color;
    StringLiteral    legend;
  };
  std::vector<Points> points;
  bool                rendered{};

  void clear() noexcept
  {
    rendered = {};
    points.clear();
  }
};

auto g_line_chart_datas = std::vector<LineChartData>{};

void init() noexcept
{
  tk::init();

  err_if(!ui::load_font("C:/Windows/Fonts/segoeui.ttf"), "failed to load font");

  g_wndCfg.display_title_bar             = true;
  g_wndCfg.display_window_shadow         = true;
  g_wndCfg.display_wireframe_only_active = true;
  g_wndCfg.wireframe_color               = 0x0000ffff;

  g_line_chart.x_tick_label_padding = { 32, 8 };
  g_line_chart.y_tick_label_padding = { 16, 32 };
  g_line_chart.axis_color           = 0xffffffff;
  g_line_chart.tick_label_size      = 12;
  g_line_chart.grid_color           = 0x404040ff;
  g_line_chart.label_size           = 14;
  g_line_chart.y_label_padding      = 4;
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

auto get_line_chart() noexcept
{
  LineChartData* data{};
  auto it = std::ranges::find_if(g_line_chart_datas, [](auto const& data) { return !data.rendered; });
  if (it == g_line_chart_datas.end())
    data = &g_line_chart_datas.emplace_back(LineChartData());
  else
    data = &*it;
  return data;
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
  get_line_chart()->points.emplace_back(xs, ys, color, legend);
}

auto to_all_vs = [](auto&& range, auto&& mem)
{
  return range | std::views::transform(mem) | std::views::join | std::ranges::to<std::vector<float>>();
};

void render_line_chart(LineChartData& data) noexcept
{
  assert(!data.rendered);
  data.rendered = true;

  auto xs = to_all_vs(data.points, &LineChartData::Points::xs);
  auto ys = to_all_vs(data.points, &LineChartData::Points::ys);
 
  auto [x_vs, x_step] = ui::get_tick_values(xs);
  auto [y_vs, y_step] = ui::get_tick_values(ys);
  auto x_ts = ui::get_tick_labels(x_vs, x_step);
  auto y_ts = ui::get_tick_labels(y_vs, y_step);
 
  g_line_chart.x_axis_label       = g_x_label;
  g_line_chart.y_axis_label       = g_y_label;
  g_line_chart.x_axis_tick_values = x_vs;
  g_line_chart.y_axis_tick_values = y_vs;
  g_line_chart.x_axis_tick_labels = x_ts;
  g_line_chart.y_axis_tick_labels = y_ts;
 
  // get draw information
  struct Legend
  {
    StringLiteral name;
    uint32_t      color;
    Rect          text_rect;
  };
  auto datas   = std::vector<ui::LineChart::Data>{};
  auto pss     = std::vector<std::vector<float2>>{};
  auto legends = std::vector<Legend>{};
  datas.reserve(data.points.size());
  pss.reserve(data.points.size());
  legends.reserve(data.points.size());
  for (auto [xs, ys, color, legend] : data.points)
  {
    pss.emplace_back(std::views::zip_transform([](auto x, auto y) { return float2{ x, y }; }, xs, ys)
      | std::ranges::to<std::vector<float2>>());
    datas.emplace_back(pss.back(), color);
    legends.emplace_back(legend, color);
  }
  g_line_chart.datas = datas;
 
  // draw line chart
  auto advice_counts = g_line_chart.calc_layout(g_limit_size);
  if (advice_counts.x)
  {
    ui::adjust_tick_values(x_vs, advice_counts.x);
    x_ts = ui::get_tick_labels(x_vs, x_step);
    g_line_chart.x_axis_tick_values = x_vs;
    g_line_chart.x_axis_tick_labels = x_ts;
  }
  if (advice_counts.y)
  {
    ui::adjust_tick_values(y_vs, advice_counts.y);
    y_ts = ui::get_tick_labels(y_vs, y_step);
    g_line_chart.y_axis_tick_values = y_vs;
    g_line_chart.y_axis_tick_labels = y_ts;
  }
  if (advice_counts.x || advice_counts.y) g_line_chart.calc_layout();
  g_line_chart.render(g_pos);
 
  // get legends max height
  auto max_height = 0.f;
  auto max_width  = 0.f;
  for (auto& [legend, color, rect] : legends)
  {
    if (auto rc = ui::get_text_bounding_rect(legend, g_line_chart.label_size))
    {
      rect = rc.value();
      max_height = std::max(max_height, rect.height());
      max_width  = std::max(max_width, rect.width());
    }
  }

  // draw legends
  auto line_chart_ext = g_line_chart.extent();
  auto padding        = 5;
  auto beg_pos        = float2{ g_pos.x + line_chart_ext.x + 10, g_pos.y };
  auto extent         = float2{ padding + max_height + padding + max_width + padding, padding };
  auto pos            = beg_pos + float2(padding);
  for (auto& [legend, color, rect] : legends)
  {
    if (!rect.empty())
    {
      ui::rectangle(pos, pos + float2(max_height), color);
      auto offset = (max_height - rect.height()) / 2;
      ui::text(legend, pos + float2{ max_height + padding, offset - rect.top }, g_line_chart.label_size, g_line_chart.axis_color);
      offset    = max_height + padding;
      pos.y    += offset;
      extent.y += offset;
    }
  }
  // draw legends border
  ui::rectangle(beg_pos, beg_pos + extent, g_line_chart.axis_color, 1);

  g_pos.y += g_line_chart.extent().y + 10;
}

void update() noexcept
{
  for (auto& data : g_line_chart_datas)
  {
    if (data.rendered) continue;
    render_line_chart(data);
  }

  ui::end();
  tk::update();

  for (auto& data : g_line_chart_datas) data.clear();
  g_pos = float2(10);
}

void new_line_chart() noexcept
{
  render_line_chart(*get_line_chart());
}

}
