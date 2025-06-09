#include "tk/Window.hpp"
#include "tk/ErrorHandling.hpp"

#include <cassert>

namespace tk {

void Window::init_key_states()
{
  using enum type::key;
  _key_states = 
  {
    { q,     {} },
    { space, {} },
  };
}

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

  init_key_states();

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

auto map_key(type::key k) noexcept -> int
{
  using enum type::key;
  switch (k)
  {
  case q:     return GLFW_KEY_Q;
  case space: return GLFW_KEY_SPACE;
  };
  assert(true);
  return -1;
}

auto Window::get_key(type::key k) noexcept -> type::key_state
{
  using enum type::key_state;

  auto  state     = glfwGetKey(_window, map_key(k));
  auto& key_state = _key_states.at(k);
  auto  now       = std::chrono::high_resolution_clock::now();

  if (key_state.state == release)
  {
    if (state == GLFW_RELEASE)
      return release;
    else
    {
      key_state.state      = press;
      key_state.start_time = now;
      key_state.last_time  = now;
      return press;
    }
  }
  else
  {
    if (state == GLFW_RELEASE)
    {
      key_state.state = release;
      return release;
    }
    else
    {
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - key_state.start_time).count();
      if (duration < _key_start_repeat_time)
      {
        return repeate_wait;
      }
      
      duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - key_state.last_time).count();
      if (duration > _key_repeat_interval)
      {
        key_state.last_time = now;
        return press;
      }
    }
  }
  assert(true);
  return {};
}

}