#include "renderer.hpp"
#include "core.hpp"
#include "engine/graphics_engine.hpp"
#include "../util/error_handling.hpp"
#include "resource/descriptor_heap_manager.hpp"

#include <chrono>

namespace tk { namespace renderer {

void Renderer::init() noexcept
{
  _thread = std::jthread([this]
  {
    g_core.init();
    g_desc_heap_mgr.init();
    g_graphics_engine.init();

    while (!_exit.load(std::memory_order_relaxed))
    {
      message_process();
      // TODO: sleep when no render task
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
  });
}

void Renderer::destroy() noexcept
{
  _exit.store(true, std::memory_order_relaxed);
  _thread.join();
}

void Renderer::add_frame_render_complete_func(std::function<void()>&& func) noexcept
{
  _frame_render_complete_funcs.emplace_back([func, last_fence_value = g_graphics_engine.signal()]()
  {
    auto fence_value = g_core.fence_completed_value();
    err_if(fence_value == UINT64_MAX, "failed to get fence value because device is removed");
    auto render_complete = fence_value >= last_fence_value;
    if (render_complete) func();
    return render_complete;
  });
}

void Renderer::message_process() noexcept
{
  for (auto it = _frame_render_complete_funcs.begin(); it != _frame_render_complete_funcs.end();)
    (*it)() ? it = _frame_render_complete_funcs.erase(it) : ++it;
}

}}
