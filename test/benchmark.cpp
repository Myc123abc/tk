#include "tk/tk.hpp"
#include "tk/error_handling.hpp"

#include <span>

using namespace tk;

struct TextLayout
{
  std::string_view text;
  float2           pos;
  float2           extent;

  void transform(ui::Transform const& transform) noexcept
  {
    auto rect = transform.transform_rect(pos, extent);
    pos       = rect.pos();
    extent    = rect.extent();
  }
};

struct TextLayoutResult
{
  std::vector<TextLayout> texts;
  float2                  extent;

  auto unit_width() const noexcept
  {
    return extent.x / texts.size();
  }

  auto unit_height() const noexcept
  {
    return extent.y / texts.size();
  }

  auto unit_extent() const noexcept
  {
    return extent / texts.size();
  }
};

void render_texts(std::span<TextLayout const> texts, float size, ui::Color color, ui::TextConfig const& cfg) noexcept
{
  for (auto const& [text, pos, _] : texts)
    ui::text(text, pos, size, color, cfg);
}

auto transform_text_layout(TextLayoutResult layout, ui::Transform const& transform) noexcept -> TextLayoutResult
{
  auto rect = Rect{};

  for (auto& text : layout.texts)
  {
    text.transform(transform);
    rect.expand(text.pos);
    rect.expand(text.pos + text.extent);
  }

  layout.extent = rect.extent();
  return layout;
}

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

void left_vertical_text(std::string_view text, float2 pos, float size, ui::Color color, ui::TextConfig const& cfg) noexcept
{
  auto ext = ui::text(text, size, cfg).extent;
  ui::transform_beg(ui::Transform{}.rotate(pos, -90).translate(0, ext.x));
  ui::text(text, pos, size, color, cfg);
  ui::transform_end();
}

void right_vertical_text(std::string_view text, float2 pos, float size, ui::Color color, ui::TextConfig const& cfg) noexcept
{
  auto ext = ui::text(text, size, cfg).extent;
  ui::transform_beg(ui::Transform{}.rotate(pos, 90).translate(ext.y, 0));
  ui::text(text, pos, size, color, cfg);
  ui::transform_end();
}

auto to_strings(std::span<float> nums) noexcept
{
  auto res = std::vector<std::string>{};
  res.reserve(nums.size());
  for (auto num : nums) res.emplace_back(std::format("{}", num));
  return res;
}

auto layout_center_texts(std::span<std::string const> texts, float2 pos, float size, ui::TextConfig const& cfg, float padding) noexcept -> TextLayoutResult
{
  auto res = TextLayoutResult{};
  res.texts.reserve(texts.size());
  auto max_len    = 0.f;
  auto max_height = 0.f;
  for (auto const& text : texts)
  {
    auto ext = ui::text(text, size, cfg).extent;
    auto len = ext.x;
    max_len = std::max(len, max_len);
    max_height = std::max(ext.y, max_height);
    res.texts.emplace_back(text, pos, ext);
  }

  auto width = max_len + padding * 2;
  auto pos_x = pos.x;
  for (auto& text : res.texts)
  {
    auto len = text.extent.x;
    auto x = pos_x + (width - len) / 2;
    text.pos = { x, pos.y };
    pos_x += width;
  }

  res.extent = { width * texts.size(), max_height };
  return res;
}

auto center_texts(std::span<std::string const> texts, float2 pos, float size, ui::Color color, ui::TextConfig const& cfg, float padding) noexcept -> TextLayoutResult
{
  auto res = layout_center_texts(texts, pos, size, cfg, padding);
  render_texts(res.texts, size, color, cfg);
  return res;
}

auto layout_left_vertical_center_texts(std::span<std::string const> texts, float2 pos, float size, ui::TextConfig const& cfg, float padding) noexcept -> TextLayoutResult
{
  auto res = layout_center_texts(texts, pos, size, cfg, padding);
  return transform_text_layout(std::move(res), ui::Transform{}.rotate(pos, -90).translate(0, res.extent.x));
}

auto left_vertical_center_texts(std::span<std::string const> texts, float2 pos, float size, ui::Color color, ui::TextConfig const& cfg, float padding) noexcept -> TextLayoutResult
{
  auto local_layout = layout_center_texts(texts, pos, size, cfg, padding);
  auto transform    = ui::Transform{}.rotate(pos, -90).translate(0, local_layout.extent.x);

  ui::transform_beg(transform);
  render_texts(local_layout.texts, size, color, cfg);
  ui::transform_end();

  return transform_text_layout(std::move(local_layout), transform);
}

auto line_chart(float2 pos, LineChartInfo const& info) noexcept -> float2
{
  auto cfg = ui::TextConfig{};

  auto size  = 12;
  auto color = 0x000000ff;

  auto x_tick_label_strs = to_strings(info.x_tick_labels);

  // draw y tick labels
  auto y_tick_label_strs = to_strings(info.y_tick_labels);
  auto y_layout = left_vertical_center_texts(y_tick_label_strs, pos, size, color, cfg, 5);
  auto y_tick_center = [](TextLayout const& text) noexcept
  {
    return text.pos.y + text.extent.y / 2;
  };
  
  // draw y axis
  auto y_axis_x = pos.x + y_layout.extent.x;
  auto o_p = float2{ y_axis_x, y_tick_center(y_layout.texts.front()) };
  auto grid_start_p = float2{ y_axis_x, y_tick_center(y_layout.texts.back()) };
  ui::line(grid_start_p, o_p, color);

  // draw x tick labels
  auto x_layout = layout_center_texts(x_tick_label_strs, o_p, size, cfg, 5);
  auto half_unit_w = x_layout.unit_width() / 2;
  for (auto& text : x_layout.texts)
    text.pos.x -= half_unit_w;
  render_texts(x_layout.texts, size, color, cfg);

  // draw x axis
  auto x_axis_width = x_layout.extent.x;
  auto grid_end_x = o_p.x + x_axis_width - half_unit_w;
  ui::line(o_p, { grid_end_x, o_p.y }, color);

  // draw grid lines
  for (auto const& text : y_layout.texts)
  {
    auto y = text.pos.y + text.extent.y / 2;
    ui::line({ y_axis_x, y }, { grid_end_x, y }, color);
  }

  // right_vertical_text(info.x_name, {}, 12, 0x000000ff, cfg);
  return {};
}

auto main() -> int
{
  tk::init();

  err_if(!ui::load_font("assets/font/SourceCodePro-Regular.ttf"), "failed to load font");

  auto wndCfg = ui::WindowConfig{};
  wndCfg.display_title_bar             = true;
  wndCfg.display_window_shadow         = true;
  wndCfg.display_wireframe_only_active = true;
  wndCfg.wireframe_color               = 0x0000ffff;

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

  auto is_closed = false;
  while (!is_closed)
  {
    ui::begin("Benchmark", 0, 0, 200, 200, &is_closed, wndCfg);

    ui::rectangle({}, ui::window_drawable_extent(), 0xffffffff);

    line_chart({}, info);

    ui::end();

    tk::update();
  }

  tk::destroy();
}
