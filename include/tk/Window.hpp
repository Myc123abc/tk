//
// window class
//
// only can generate single window
//
// use SDL_GetDisplayContentScale can get display scale, and have SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED event
// but I don't care scale ui. I want to keep ui size in different scale
//

#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <string_view>

struct SDL_Window;

#define GET_DPI_IMPL 0

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

#if GET_DPI_IMPL
    auto get_dpi() const noexcept -> uint32_t;
#endif

  private:
    SDL_Window* _window;
  };

}
