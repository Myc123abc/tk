#include "engine.hpp"
#include "../core.hpp"
#include "../../util/error_handling.hpp"

namespace tk { namespace renderer {

void Engine::init(D3D12_COMMAND_LIST_TYPE type) noexcept
{
  auto device = g_core.device();

  // create command queue
  D3D12_COMMAND_QUEUE_DESC queue_desc{};
  queue_desc.Type = type;
  err_if(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&_queue)),
          "failed to create command queue");
  
  // create command allocator and list
  err_if(device->CreateCommandAllocator(type, IID_PPV_ARGS(&_cmd_alloc)),
          "failed to create command allocator");
  err_if(device->CreateCommandList(0, type, _cmd_alloc.Get(), nullptr, IID_PPV_ARGS(&_cmd)),
          "failed to create command list");
  err_if(_cmd->Close(), "failed to close command list");

  // create fence
  err_if(g_core.device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)),
          "failed to create fence");
//   _fence_event = CreateEvent(nullptr, false, false, nullptr);
//   err_if(!_fence_event, "failed to create fence event");
//   err_if(!CloseHandle(_fence_event), "failed to destroy fence event");
}

auto Engine::signal() noexcept -> uint64_t
{
  err_if(_queue->Signal(_fence.Get(), ++_fence_value), "failed to signal fence");
  return _fence_value;
}

}}
