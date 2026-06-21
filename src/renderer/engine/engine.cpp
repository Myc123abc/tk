#include "engine.hpp"
#include "../core.hpp"
#include "util/error_handling.hpp"

namespace tk::renderer {

void Engine::init(D3D12_COMMAND_LIST_TYPE type) noexcept
{
  auto device = g_core.device();

  // create command queue
  _queue.init(type);
  
  // create command allocator and list
  _cmd_alloc = g_core.create_cmd_alloc(type);
  _cmd       = g_cmd_pool.create(type, _cmd_alloc.Get());

  // create fence
  err_if(g_core.device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)),
          "failed to create fence");
  _fence_event = CreateEventW(nullptr, false, false, nullptr);
  err_if(!_fence_event, "failed to create win32 event");
}

auto Engine::signal() noexcept -> uint64
{
  _queue.signal(_fence.Get(), ++_fence_value);
  return _fence_value;
}

auto Engine::submit() noexcept -> uint64
{
  _queue.submit(_fence.Get(), ++_fence_value, { _cmd });
  return _fence_value;
}

void Engine::wait(Engine const& engine, std::optional<uint64> fence_value) const noexcept
{
  _queue.wait(engine._fence.Get(), fence_value.value_or(engine._fence_value));
}

auto Engine::set_event_on_completion() const noexcept -> HANDLE
{
  err_if(_fence->SetEventOnCompletion(_fence_value, _fence_event), "failed to set event on completion");
  return _fence_event;
}

void Engine::wait_idle() noexcept
{
  err_if(_fence->SetEventOnCompletion(signal(), _fence_event), "failed to set event on completion");
  WaitForSingleObjectEx(_fence_event, INFINITE, false);
}

void Engine::destroy() noexcept
{
  CloseHandle(_fence_event);
  g_cmd_pool.destroy(_cmd);
  _queue.destroy();
}

}
