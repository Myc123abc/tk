#pragma once

#include "ui.hpp"
#include "text_layout.hpp"

namespace tk::ui {

struct LineChart
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
  std::string_view             font_family;
  FontStyle                    font_style{};

  struct Data
  {
    std::span<float2> points;
    Color             color;
  };
  std::span<Data> datas;

  // Calculate layout of the line chart.
  void calc_layout() noexcept;

  /**
   * Render line chart.
   * @param pos
   */
  void render(float2 pos) noexcept;

  auto extent() const noexcept { return _layout.extent; }

private:
  struct Layout
  {
    TextLayout y_layout;
    TextLayout x_layout;
    float      x_label_width{};
    float2     extent;
    float      x_unit_interval_len{};
    float      y_unit_interval_len{};
    float      x_label_y_offset{};
  } _layout;
};

/**
 * Split x values and y values from points.
 * @param ps points
 * @return x values and y values
 */
auto split_x_y_values(std::span<float2> ps) noexcept -> std::pair<std::vector<float>, std::vector<float>>;

/**
 * Convert values to tick values with speicifc count (count maybe adjusted).
 * @param vs values
 * @param cnt count
 * @return tick values and step value
 */
auto get_tick_values(std::span<float> vs, uint cnt) noexcept -> std::pair<std::vector<float>, uint>;

/**
 * Convert tick values to format string.
 * @param vs tick values
 * @param step
 * @return tick format strings
 */
auto get_tick_labels(std::span<float> vs, float step) noexcept -> std::vector<std::string>;

}
