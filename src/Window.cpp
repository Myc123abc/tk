#include "tk/Window.hpp"
#include "tk/ErrorHandling.hpp"

#include <cassert>

namespace tk {

auto Window::init(std::string_view title, uint32_t width, uint32_t height) -> Window&
{
  // HACK: expand to multi-windows manage
  static bool first = true;
  assert(first);
  if (first)  first = false;

  assert(width > 0 && height > 0);

  _width  = width;
  _height = height;

#ifdef _WIN32
  // use branch unblock_events_windows_move_resize of mmozeiko/glfw
  glfwInitHint(GLFW_WIN32_MESSAGES_IN_FIBER, GLFW_TRUE);
#endif
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_VISIBLE , GLFW_FALSE);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  _window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
  throw_if(!_window, "failed to create window");

  glfwSetWindowUserPointer(_window, this);

  return *this;
}

void Window::destroy() const
{
  glfwDestroyWindow(_window);
  glfwTerminate();
}

auto Window::show() -> Window&
{
  glfwShowWindow(_window);
  return *this;
}

auto Window::create_surface(VkInstance instance) const -> VkSurfaceKHR
{
  VkSurfaceKHR surface;
  throw_if(glfwCreateWindowSurface(instance, _window, nullptr, &surface) != VK_SUCCESS,
           "failed to create vulkan surface");
  return surface;
}

auto Window::get_framebuffer_size() const -> glm::vec2
{
  int w, h;
  glfwGetFramebufferSize(_window, &w, &h);
  return { w, h };
}
    
auto Window::is_resized() noexcept -> bool
{
  auto size = get_framebuffer_size();
  if (_width != size.x || _height != size.y)
  {
    _width  = size.x;
    _height = size.y;
    return true;
  }
  return false;
}

auto Window::get_vulkan_instance_extensions() -> std::vector<const char*>
{
  uint32_t count;
  auto ret = glfwGetRequiredInstanceExtensions(&count);
  throw_if(!ret, "failed to get instance extensions of vulkan");
  return std::vector(ret, ret + count);
}

auto Window::get_cursor_position() const noexcept -> glm::vec2
{
  double x, y;
  glfwGetCursorPos(_window, &x, &y);
  return { x, y };
}

auto Window::get_mouse_state() const noexcept -> type::mouse
{
  auto res = glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_LEFT);
  return res == GLFW_PRESS ? type::mouse::left_down : type::mouse::left_up;
}

}