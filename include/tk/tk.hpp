#pragma once

#include "util/log.hpp"
#include "ui/ui.hpp"
#include "ui/lerpolator.hpp"
#include "util/log.hpp"

namespace tk {

// initialize tk library
void init() noexcept;

// destroy the tk library
void destroy() noexcept;

// process window message and renderer rendering
void update() noexcept;

}
