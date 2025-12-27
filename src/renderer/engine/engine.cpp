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
  _cmd_alloc = g_core.create_cmd_alloc(type);
  _cmd       = g_core.create_cmd(type, _cmd_alloc.Get());

  // create fence
  err_if(g_core.device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)),
          "failed to create fence");
  _fence_event = CreateEventA(nullptr, false, false, nullptr);
  err_if(!_fence_event, "failed to create win32 event");
}

auto Engine::signal() noexcept -> uint64_t
{
  err_if(_queue->Signal(_fence.Get(), ++_fence_value), "failed to signal fence");
  return _fence_value;
}

auto Engine::submit(std::initializer_list<ID3D12GraphicsCommandList*> cmds) noexcept -> uint64_t
{
  for (auto const& cmd : cmds)
    err_if(cmd->Close(), "failed to close command list");
  _queue->ExecuteCommandLists(cmds.size(), reinterpret_cast<ID3D12CommandList* const*>(cmds.begin()));
  return signal();
}

auto Engine::set_event_on_completion() const noexcept -> HANDLE
{
  err_if(_fence->SetEventOnCompletion(_fence_value, _fence_event), "failed to set event on completion");
  return _fence_event;
}

void Engine::destroy() noexcept
{
  err_if(_fence->SetEventOnCompletion(signal(), _fence_event), "failed to set event on completion");
  WaitForSingleObjectEx(_fence_event, INFINITE, false);
  CloseHandle(_fence_event);
}

}}
