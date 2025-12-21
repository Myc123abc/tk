#include "renderer.hpp"
#include "core.hpp"
#include "engine/graphics_engine.hpp"
#include "engine/copy_engine.hpp"
#include "../util/error_handling.hpp"
#include "resource/descriptor_heap_manager.hpp"
#include "compiler.hpp"

#include <chrono>

namespace tk { namespace renderer {

void Renderer::init() noexcept
{
  _thread = std::jthread([this]
  {
    g_core.init();
    g_compiler.init();
    g_desc_heap_mgr.init();
    g_graphics_engine.init();
    g_copy_engine.init();

    _sdf_pipeline.init_graphics("assets/shader/sdf.hlsl", "vs", "ps", "assets/shader", RenderResource::Render_Target_Format, true);

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
  while (!_frame_render_complete_funcs.empty() || !_msg_queue.empty())
    message_process();
}

// HACK: can make fence more detail, this resource is used by which engines
void Renderer::add_frame_render_complete_func(std::function<void()>&& func) noexcept
{
  auto last_fence_values = std::vector<std::pair<Engine&, uint64_t>>
  {
    { g_graphics_engine, g_graphics_engine.signal() },
    { g_copy_engine,     g_copy_engine.signal() },
  };
  _frame_render_complete_funcs.emplace_back([func, last_fence_values = std::move(last_fence_values)]()
  {
    for (auto [engine, last_fence_value] : last_fence_values)
    {
      auto fence_value = engine.fence_completed_value();
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

  while (!_msg_queue.empty())
  {
    (*_msg_queue.front())();
    _msg_queue.pop();
  }
}

void Renderer::msg_create_window_resource(HWND handle, uint32_t width, uint32_t height) noexcept
{
  err_if(_res.contains(handle), "failed to create window render resource, it's already exist");
  auto res = RenderResource{};
  res.init(handle, width, height);
  _res.emplace(handle, std::move(res));
}

void Renderer::msg_destroy_window_resource(HWND handle) noexcept
{
  err_if(!_res.contains(handle), "failed to destroy window render resource, it's unexist");
  add_frame_render_complete_func([handle, res = std::move(_res.at(handle))] mutable
  {
    res.destroy();
    DestroyWindow(handle);
  });
  _res.erase(handle);
}

/*

TODO:
per image per graphics and copy command lists
only wait last frame of window when only single window
no window render then sleep
multi-windows render can skip this frame which window is not finish render complete using frame
singal queue once and execute all windows' command lists per frame (which aslo some window's command list can be discard in current frame)

*/

}}
