#pragma once

#include "engine/engine.hpp"

#include <atomic>
#include <thread>
#include <functional>
#include <deque>
#include <initializer_list>

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

  // TODO: image, bitmap view use mdspan?

  void add_frame_render_complete_func(std::function<void()>&& func, std::initializer_list<Engine*> engines) noexcept;

private:
  std::jthread                      _thread;
  std::atomic_bool                  _exit{};

  std::deque<std::function<bool()>> _frame_render_complete_funcs;
};

inline static auto& g_renderer{ Renderer::instance() };

}}
