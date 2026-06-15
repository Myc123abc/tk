#pragma once

namespace tk::renderer {

/// track which engines will use this resource
class ResourceTrack
{
public:
  ResourceTrack()                                = default;
  ~ResourceTrack()                               = default;
  ResourceTrack(ResourceTrack const&)            = delete;
  ResourceTrack(ResourceTrack&&)                 = delete;
  ResourceTrack& operator=(ResourceTrack const&) = delete;
  ResourceTrack& operator=(ResourceTrack&&)      = delete;

  void graphics_will_use() noexcept { _graphics_will_use = true; }
  void compute_will_use()  noexcept { _compute_will_use  = true; }
  void copy_will_use()     noexcept { _copy_will_use     = true; }

  auto needs_graphics() const noexcept { return _graphics_will_use; }
  auto needs_compute()  const noexcept { return _compute_will_use;  }
  auto needs_copy()     const noexcept { return _copy_will_use;     }

  void reset_resource_track() noexcept
  {
    _graphics_will_use = {};
    _compute_will_use  = {};
    _copy_will_use     = {};
  }

private:
  bool _graphics_will_use{};
  bool _compute_will_use{};
  bool _copy_will_use{};
};

}
