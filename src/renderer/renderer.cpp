#include "renderer.hpp"
#include "device.hpp"
#include "engine/graphics_engine.hpp"
#include "engine/copy_engine.hpp"
#include "../util/error_handling.hpp"
#include "resource/descriptor_heap_manager.hpp"

#include <chrono>
#include <ranges>

namespace tk { namespace renderer {

void Renderer::init() noexcept
{
  _thread = std::jthread([this]
  {
    g_device.init();
    g_desc_heap_mgr.init();
    g_graphics_engine.init();
    g_copy_engine.init();

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

void Renderer::add_frame_render_complete_func(std::function<void()>&& func, std::initializer_list<Engine*> engines) noexcept
{
  auto last_fence_values = engines
    | std::views::transform([](auto& engine) { return std::make_pair(engine, engine->signal()); })
    | std::ranges::to<std::vector<std::pair<Engine*, uint64_t>>>();
  _frame_render_complete_funcs.emplace_back([func, last_fence_values = std::move(last_fence_values)]()
  {
    for (auto [engine, last_fence_value] : last_fence_values)
    {
      auto fence_value = engine->fence_completed_value();
      err_if(fence_value == UINT64_MAX, "failed to get fence value because device is removed");
      if (fence_value < last_fence_value) return false;
    }
    func();
    return true;
  });
}

void Renderer::message_process() noexcept
{
  for (auto it = _frame_render_complete_funcs.begin(); it != _frame_render_complete_funcs.end();)
    (*it)() ? it = _frame_render_complete_funcs.erase(it) : ++it;
}

/*

TODO:
per window per offscreen image
per image per command lists
only wait last frame of window when only single window
no window render then sleep
multi-windows render can skip this frame which window is not finish render complete using frame
singal queue once and execute all windows' command lists per frame (which aslo some window's command list can be discard in current frame)

*/

}}
