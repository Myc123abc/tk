//
// window class
//
// only can generate single window
//
// TODO:
// expand to window manager
// one init sdl3, multi-generate windows
//
// HACK:
// 1. on windows, event loop resize event only tringle once, use callback system is normal...
// 2. in wayland, sdl3 defaul use x11, which resize window is suck.
//    forcely use wayland the framebuffer size return 16384x16384 out of device memory...
//

#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <string_view>

struct SDL_Window;

namespace tk {

  class Window
  {
  public:
    Window()  = default;
    ~Window() = default;

    Window(Window const&)            = delete;
    Window(Window&&)                 = delete;
    Window& operator=(Window const&) = delete;
    Window& operator=(Window&&)      = delete;

    auto init(std::string_view title, uint32_t width, uint32_t height) -> Window&;
    void destroy() const;

    auto show() -> Window&;

    auto create_surface(VkInstance instance)                     const -> VkSurfaceKHR;
    void get_framebuffer_size(uint32_t& width, uint32_t& height) const;
    void get_screen_size(uint32_t& width, uint32_t& height)      const;
    auto is_closed()                                             const -> int;
    
    static void process_events();
    static auto get_vulkan_instance_extensions() -> std::vector<const char*>;

    auto get() const noexcept { return _window; }

  private:
    SDL_Window* _window;
  };

}
