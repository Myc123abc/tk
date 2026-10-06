#pragma once

#include <span>
#include "tk/string_id.hpp"

namespace tk::benchmark {

void init() noexcept;

void destroy() noexcept;

auto running() noexcept -> bool;
void update() noexcept;

void set_labels(StringLiteral x_label, StringLiteral y_label) noexcept;
void limit_size(float width, float height) noexcept;

void present(std::span<float> xs, std::span<float> ys, uint32_t color, StringLiteral legend) noexcept;

void new_line_chart() noexcept;

}
