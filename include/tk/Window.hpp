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
#include <unordered_map>
#include <chrono>

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

    auto get_key(type::key k) noexcept -> type::key_state;

    void set_key_start_repeat_time(uint32_t time) noexcept { _key_start_repeat_time = time; }
    void set_key_repeat_interval(uint32_t time)   noexcept { _key_repeat_interval = time;   }

  private:
    GLFWwindow* _window{};
    uint32_t    _width{}, _height{};

    struct KeyState
    {
      std::chrono::high_resolution_clock::time_point start_time;
      std::chrono::high_resolution_clock::time_point last_time;
      type::key_state state{ type::key_state::release };
    };
    std::unordered_map<type::key, KeyState> _key_states;
    uint32_t _key_start_repeat_time{ 400 };
    uint32_t _key_repeat_interval{ 80 };
  
    void init_key_states();
  };

}
