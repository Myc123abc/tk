//
// window class
//
// only can generate single window
//
// TODO:
// expand to window manager
// one init glfw, multi-generate windows
//

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <string_view>

namespace tk
{
  class Window final
  {
  public:
    Window(uint32_t width, uint32_t height, std::string_view title);
    ~Window();

    Window(Window const&)            = delete;
    Window(Window&&)                 = delete;
    Window& operator=(Window const&) = delete;
    Window& operator=(Window&&)      = delete;

    auto create_surface(VkInstance instance)           const -> VkSurfaceKHR;
    void get_framebuffer_size(int& width, int& height) const;
    auto is_closed()                                   const -> int;
    
    static void process_events();
    static auto get_vulkan_instance_extensions() -> std::vector<const char*>;

  private:
    GLFWwindow* _window;
  };
}
