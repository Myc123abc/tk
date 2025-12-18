#include "graphics_engine.hpp"
#include "../core.hpp"
#include "../../util/error_handling.hpp"

#include <array>

namespace tk { namespace renderer {

void GraphicsEngine::init() noexcept
{
  auto device = g_core.device();

  // create command queue
  D3D12_COMMAND_QUEUE_DESC queue_desc{};
  err_if(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&_queue)),
          "failed to create command queue");
  
  // create command allocator and list
  err_if(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmd_alloc)),
          "failed to create command allocator");
  err_if(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmd_alloc.Get(), nullptr, IID_PPV_ARGS(&_cmd)),
          "failed to create command list");
  err_if(_cmd->Close(), "failed to close command list");
}

auto GraphicsEngine::signal() noexcept -> uint64_t
{
  return g_core.signal(_queue.Get());
}

}}
