#include "tk/Window.hpp"
#include "tk/ErrorHandling.hpp"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_hints.h>

#include <cassert>

namespace tk {

auto Window::init(std::string_view title, uint32_t width, uint32_t height) -> Window&
{
  // HACK: expand to multi-windows manage
  static bool first = true;
  assert(first);
  if (first)  first = false;

  SDL_Init(SDL_INIT_VIDEO);

  // Set SDL hint to receive mouse click events on window focus, otherwise SDL doesn't emit the event.
  // Without this, when clicking to gain focus, our widgets wouldn't activate even though they showed as hovered.
  // (This is unfortunately a global SDL setting, so enabling it might have a side-effect on your application.
  // It is unlikely to make a difference, but if your app absolutely needs to ignore the initial on-focus click:
  // you can ignore SDL_EVENT_MOUSE_BUTTON_DOWN events coming right after a SDL_EVENT_WINDOW_FOCUS_GAINED)
  SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
  
  // Disable auto-capture, this is preventing drag and drop across multiple windows
  SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "0");

  // see https://github.com/libsdl-org/SDL/issues/6659
  SDL_SetHint("SDL_BORDERLESS_WINDOWED_STYLE", "0");

  auto flags = SDL_WINDOW_VULKAN             |
               SDL_WINDOW_RESIZABLE          |
               SDL_WINDOW_HIGH_PIXEL_DENSITY |
               SDL_WINDOW_HIDDEN;
  _window = SDL_CreateWindow(title.data(), width, height, flags);
  throw_if(!_window, "failed to create window!");

  return *this;
}

void Window::destroy() const
{
  SDL_DestroyWindow(_window);
  SDL_Quit();
}

auto Window::show() -> Window&
{
  SDL_SetWindowPosition(_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(_window);
  return *this;
}

auto Window::create_surface(VkInstance instance) const -> VkSurfaceKHR
{
  VkSurfaceKHR surface;
  throw_if(!SDL_Vulkan_CreateSurface(_window, instance, nullptr, &surface),
           "failed to create surface");
  return surface;
}

// TODO:
// io.DisplayFramebufferScale = ImVec2((float)display_w / w, (float)display_h / h);
// imgui use "window size" / "pixel size" get scale

/*
TODO:
// Update monitors
#ifdef WIN32
    bd->WantUpdateMonitors = true; // Keep polling under Windows to handle changes of work area when resizing task-bar (#8415)
#endif
*/

void Window::get_framebuffer_size(uint32_t& width, uint32_t& height) const
{
  SDL_GetWindowSizeInPixels(_window, (int*)&width, (int*)&height);
}

void Window::get_screen_size(uint32_t& width, uint32_t& height) const
{
  auto id   = SDL_GetDisplayForWindow(_window);
  auto mode = SDL_GetCurrentDisplayMode(id);
  width  = mode->w;
  height = mode->h;
}
    
auto Window::get_vulkan_instance_extensions() -> std::vector<const char*>
{
  uint32_t count;
  auto ret = SDL_Vulkan_GetInstanceExtensions(&count);
  return std::vector(ret, ret + count);
}

}
