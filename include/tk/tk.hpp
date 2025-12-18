#pragma once

#include <stdint.h>

namespace tk {

void init() noexcept;

void destroy() noexcept;

auto window_count() noexcept -> uint32_t;

void test();

}
