#include "tk/tk.hpp"
#include "tk/error_handling.hpp"
#include "tk/ui/line_chart.hpp"

using namespace tk;

auto main() -> int
{
  tk::init();

  err_if(!ui::load_font("assets/font/NotoSansJP-Regular.ttf"), "failed to load font");

  auto wndCfg = ui::WindowConfig{};
  wndCfg.display_title_bar             = true;
  wndCfg.display_window_shadow         = true;
  wndCfg.display_wireframe_only_active = true;
  wndCfg.wireframe_color               = 0x0000ffff;

  auto data1 = std::vector<float2>
  {
    { -1, -1 },
    { 0, 0 },
    { 1, 3 },
    { 2, 7 },
    { 3, 12 },
    { 4.5, 18 },
    { 5, 30 },
    { 7, 45.6 },
  };
  auto data2 = std::vector<float2>
  {
    { 0, 0 },
    { 1, 5 },
    { 2, 9 },
    { 3, 11 },
    { 4, 25 },
    { 5, 30 },
    { 6, 14 },
  };
  auto data = std::vector<ui::LineChart::Data>
  {
    { data1, 0x0000ffff },
    { data2, 0xff0000ff },
  };

  auto [x_vs, y_vs]     = ui::split_x_y_values(data1);
  auto [x_t_vs, x_step] = ui::get_tick_values(x_vs, x_vs.size());
  auto [y_t_vs, y_step] = ui::get_tick_values(y_vs, y_vs.size());
  auto x_t_ls           = ui::get_tick_labels(x_t_vs, x_step);
  auto y_t_ls           = ui::get_tick_labels(y_t_vs, y_step);

  auto line_chart = ui::LineChart{};
  line_chart.x_axis_label = "Time (s)";
  line_chart.y_axis_label = "Speed (m\\s)";
  line_chart.x_axis_tick_values = x_t_vs;
  line_chart.y_axis_tick_values = y_t_vs;
  line_chart.x_axis_tick_labels = x_t_ls;
  line_chart.y_axis_tick_labels = y_t_ls;
  line_chart.x_tick_label_padding = { 32, 8 };
  line_chart.y_tick_label_padding = { 8, 32 };
  line_chart.datas = data;
  line_chart.axis_color = 0xffffffff;
  line_chart.tick_label_size = 12;
  line_chart.grid_color = 0x404040ff;
  line_chart.label_size = 14;
  line_chart.y_label_padding = 4;

  auto is_closed = false;
  while (!is_closed)
  {
    ui::begin("Benchmark", 0, 0, 200, 200, &is_closed, wndCfg);

    ui::rectangle({}, ui::window_drawable_extent(), 0x000000ff);

    auto pos = float2(10);
    line_chart.calc_layout();
    line_chart.render(pos);
    ui::rectangle(pos, pos + line_chart.extent(), 0x00ff00ff, 1);

    ui::end();

    tk::update();
  }

  tk::destroy();
}
