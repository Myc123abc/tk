#include "resource_tack.hpp"

#include "../engine/graphics_engine.hpp"
#include "../engine/compute_engine.hpp"
#include "../engine/copy_engine.hpp"

namespace tk::renderer {

auto ResourceTrack::graphics_used() const noexcept -> bool { return g_graphics_engine.fence_completed_value() < graphics_fence_value; }
auto ResourceTrack::compute_used()  const noexcept -> bool { return g_comp_engine.fence_completed_value()     < compute_fence_value;  }
auto ResourceTrack::copy_used()     const noexcept -> bool { return g_copy_engine.fence_completed_value()     < copy_fence_value;     }

}
