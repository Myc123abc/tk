#pragma once

#include "util/base.hpp"

namespace tk::renderer {

/// track which engines using this resource
class ResourceTrack
{
public:
  ResourceTrack()                                = default;
  ~ResourceTrack()                               = default;
  ResourceTrack(ResourceTrack const&)            = delete;
  ResourceTrack(ResourceTrack&&)                 = delete;
  ResourceTrack& operator=(ResourceTrack const&) = delete;
  ResourceTrack& operator=(ResourceTrack&&)      = delete;

  auto graphics_used() const noexcept -> bool;
  auto compute_used()  const noexcept -> bool;
  auto copy_used()     const noexcept -> bool;

  auto engines_used() const noexcept { return graphics_used() || compute_used() || copy_used(); }

private:
  uint64 graphics_fence_value{};
  uint64 compute_fence_value{};
  uint64 copy_fence_value{};
};

}
