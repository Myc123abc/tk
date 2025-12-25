#pragma once

#include "resource/render_data.hpp"
#include "resource/render_resource.hpp"
#include "pipeline.hpp"
#include "config.hpp"

#include <rigtorp/SPSCQueue.h>

#include <atomic>
#include <thread>
#include <functional>
#include <deque>
#include <unordered_map>
#include <span>

namespace tk { namespace renderer {

class Renderer
{
private:
  Renderer()                           = default;
  ~Renderer()                          = default;
public:
  Renderer(Renderer const&)            = delete;
  Renderer(Renderer&&)                 = delete;
  Renderer& operator=(Renderer const&) = delete;
  Renderer& operator=(Renderer&&)      = delete;

  static auto instance() noexcept -> Renderer&
  {
    static Renderer instance;
    return instance;
  }

  void init() noexcept;

  void destroy() noexcept;

  void message_process() noexcept;

  void add_frame_render_complete_func(std::move_only_function<void()>&& func) noexcept;

  auto render(HWND handle, RenderData* render_data) noexcept
  {
    if (_render_datas.try_emplace(handle, render_data))
    {
      render_data->use();
      return true;
    }
    return false;
  }

private:
  void render() noexcept;
  void render_sdf(RenderResource& res, std::span<Vertex const> vertices, std::span<uint16_t const> indices, std::span<ShapeProperty const> shape_properties) noexcept;

  //
  // message process
  //
public:
  void create_window_resource(HWND handle, uint32_t width, uint32_t height) noexcept
  { _msg_queue.emplace([this, handle, width, height] { msg_create_window_resource(handle, width, height); }); }
  void destroy_window_resource(HWND handle) noexcept
  { _msg_queue.emplace([this, handle] { msg_destroy_window_resource(handle); }); }

private:
  void msg_create_window_resource(HWND handle, uint32_t width, uint32_t height) noexcept;
  void msg_destroy_window_resource(HWND handle) noexcept;

private:
  std::jthread                                        _thread;
  std::atomic_bool                                    _exit{};

  std::deque<std::move_only_function<bool()>>         _frame_render_complete_funcs;
  rigtorp::SPSCQueue<std::move_only_function<void()>> _msg_queue{ Renderer_Msg_Queue_Size };

  std::unordered_map<HWND, RenderResource>            _res;
  Pipeline                                            _sdf_pipeline;

  rigtorp::SPSCQueue<std::pair<HWND, RenderData*>>    _render_datas{ Render_Data_Queue_Size };
};

inline static auto& g_renderer{ Renderer::instance() };

}}
