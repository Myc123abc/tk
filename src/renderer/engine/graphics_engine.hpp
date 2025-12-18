#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include <stdint.h>

namespace tk { namespace renderer {

class GraphicsEngine
{
private:
  GraphicsEngine()                                 = default;
  ~GraphicsEngine()                                = default;
public:
  GraphicsEngine(GraphicsEngine const&)            = delete;
  GraphicsEngine(GraphicsEngine&&)                 = delete;
  GraphicsEngine& operator=(GraphicsEngine const&) = delete;
  GraphicsEngine& operator=(GraphicsEngine&&)      = delete;

  static auto instance() noexcept -> GraphicsEngine&
  {
    static GraphicsEngine instance;
    return instance;
  }

  void init() noexcept;

  auto signal() noexcept -> uint64_t;

private:
  Microsoft::WRL::ComPtr<ID3D12CommandQueue>         _queue;
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator>     _cmd_alloc;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList1> _cmd;
};

inline static auto& g_graphics_engine{ GraphicsEngine::instance() };

}}
