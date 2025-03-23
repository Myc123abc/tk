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

#include <string_view>

namespace tk
{
  class Window final
  {
  public:
    Window(uint32_t width, uint32_t height, std::string_view title);
    ~Window();

    Window(const Window&)            = delete;
    Window(Window&&)                 = delete;
    Window& operator=(const Window&) = delete;
    Window& operator=(Window&&)      = delete;

  private:
    GLFWwindow* _window;
  };
}
