#pragma once

#include <wrl/client.h>
#include <d3d12.h>

namespace tk { namespace renderer {

class Device
{
private:
  Device()                         = default;
  ~Device()                        = default;
public:
  Device(Device const&)            = delete;
  Device(Device&&)                 = delete;
  Device& operator=(Device const&) = delete;
  Device& operator=(Device&&)      = delete;

  static auto instance() noexcept -> Device&
  {
    static Device instance;
    return instance;
  }

  void init() noexcept;
  
  auto get() const noexcept { return _device.Get(); }

private:
  Microsoft::WRL::ComPtr<ID3D12Device2> _device;
};

inline static auto& g_device{ Device::instance() };

}}
