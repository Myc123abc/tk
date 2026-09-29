#pragma once

#include <span>
#include <string_view>

namespace tk::benchmark {

void init() noexcept;

void destroy() noexcept;

auto running() noexcept -> bool;
void update() noexcept;

void set_labels(std::string_view x_label, std::string_view y_label) noexcept;

void present(std::span<float> xs, std::span<float> ys, uint32_t color) noexcept;

void new_line_chart() noexcept;

}
