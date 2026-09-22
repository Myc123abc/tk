#include "ui/line_chart.hpp"
#include "text_engine/text_layout.hpp"
#include "ui_context.hpp"

namespace tk::ui {

struct LineChartInfo
{
  std::string_view x_name;
  std::string_view y_name;
  std::span<float> x_tick_labels;
  std::span<float> y_tick_labels;

  struct Data
  {
    std::span<float> numbers;
  };
  std::span<Data> data;
};

auto to_strings(std::span<float> nums) noexcept
{
  auto res = std::vector<std::string>{};
  res.reserve(nums.size());
  for (auto num : nums) res.emplace_back(std::format("{}", num));
  return res;
}

auto line_chart(float2 pos, LineChartInfo const& info) noexcept -> float2
{
  auto cfg = ui::TextConfig{};

  auto size  = 12;
  auto color = 0x000000ff;

  auto x_tick_label_strs = to_strings(info.x_tick_labels);

  // draw y tick labels
  auto y_tick_label_strs = to_strings(info.y_tick_labels);

  auto y_layout = TextLayout(y_tick_label_strs);
  y_layout.set_color(0xffffffff);
  y_layout.adjust_size(12);
  y_layout.set_pos({});
  y_layout.set_padding_x(4);

  g_ui_ctx.render(y_layout.center_alignment(TextLayout::Direction::vertical, TextLayout::TextOrder::reverse));

  return {};
}

void test_line_chart() noexcept
{
  auto x_tick_labels = std::vector<float>{ 1, 2.4, 3.1, 4, 5 };
  auto y_tick_labels = std::vector<float>{ 10, 20, 30, 40, 50 };
  auto data1 = std::vector<float>{ 5, 12, 33, 45, 49 };
  auto data  = std::vector{ LineChartInfo::Data{ data1 } };
  auto info = LineChartInfo
  {
    "x_name", "y_name",
    x_tick_labels,
    y_tick_labels,
    data
  };

  line_chart({}, info);
}

}
