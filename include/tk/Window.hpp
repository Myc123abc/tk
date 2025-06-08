//
// window class
//
// only can generate single window
//
// use SDL_GetDisplayContentScale can get display scale, and have SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED event
// but I don't care scale ui. I want to keep ui size in different scale
//

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "tk/type.hpp"
#include <vector>
#include <string_view>

#include <glm/glm.hpp>

namespace tk {

  inline void poll_events()
  {
    glfwPollEvents();
  }

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

    auto create_surface(VkInstance instance) const -> VkSurfaceKHR;

    auto get_framebuffer_size() const -> glm::vec2;
    
    static void process_events();
    static auto get_vulkan_instance_extensions() -> std::vector<const char*>;

    auto get() const noexcept { return _window; }

    auto get_cursor_position() const noexcept -> glm::vec2;

    auto get_mouse_state() const noexcept -> type::mouse;

    auto is_closed() const noexcept
    {
      return glfwWindowShouldClose(_window);
    }

    auto is_resized() noexcept -> bool;

    auto is_minimized() const noexcept
    {
      return glfwGetWindowAttrib(_window, GLFW_ICONIFIED);
    }

    auto is_maximized() const noexcept
    {
      return glfwGetWindowAttrib(_window, GLFW_MAXIMIZED);
    }

  private:
    GLFWwindow* _window{};
    uint32_t    _width{}, _height{};
  };

}
