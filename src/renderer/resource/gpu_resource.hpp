#pragma once

#include <assert.h>

namespace tk::renderer {

class GPUResource
{
public:
  GPUResource()                              = default;
  ~GPUResource()                             = default;
  GPUResource(GPUResource const&)            = delete;
  GPUResource(GPUResource&&)                 = delete;
  GPUResource& operator=(GPUResource const&) = delete;
  GPUResource& operator=(GPUResource&&)      = delete;

private:
};

enum class GPUResourceAccess
{
  read  = 0b0,
  write = 0b1,
};

struct GPUResourceUsage
{
  GPUResource*      resource{};
  GPUResourceAccess access;
};

constexpr auto needs_sync(GPUResourceAccess lhs, GPUResourceAccess rhs) noexcept
{
  return lhs == GPUResourceAccess::write || rhs == GPUResourceAccess::write;
}

constexpr auto needs_sync(GPUResourceUsage const& lhs, GPUResourceUsage const& rhs) noexcept
{
  assert(lhs.resource != nullptr);
  return lhs.resource == rhs.resource && needs_sync(lhs.access, rhs.access);
}

constexpr auto combine(GPUResourceAccess lhs, GPUResourceAccess rhs) noexcept
{
  return static_cast<GPUResourceAccess>(needs_sync(lhs, rhs));
}

}
