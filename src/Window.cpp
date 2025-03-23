#include "Window.hpp"
#include "ErrorHandling.hpp"

#include <assert.h>

namespace tk
{

Window::Window(uint32_t width, uint32_t height, std::string_view title)
{
  // HACK: expand to multi-windows manage
  static bool first = true;
  assert(first);
  if (first)  first = false;

  throw_if(glfwInit() == GLFW_FALSE, "failed to init GLFW!");

  // don't create OpenGL context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  _window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
  throw_if(_window == nullptr, "failed to create window!");
}

Window::~Window()
{
  glfwDestroyWindow(_window);
  glfwTerminate();
}

}
