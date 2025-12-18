#pragma once

#include <atomic>
#include <thread>
#include <functional>
#include <deque>

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

  // TODO: sperate to graphics engine and copy engine funcs?
  void add_frame_render_complete_func(std::function<void()>&& func) noexcept;

private:
  std::jthread                      _thread;
  std::atomic_bool                  _exit{};

  std::deque<std::function<bool()>> _frame_render_complete_funcs;
};

inline static auto& g_renderer{ Renderer::instance() };

}}
