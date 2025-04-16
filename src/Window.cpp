#include "tk/Window.hpp"
#include "tk/ErrorHandling.hpp"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_vulkan.h>

#include <cassert>

namespace tk {

void Window::init(std::string_view title, uint32_t width, uint32_t height)
{
  // HACK: expand to multi-windows manage
  static bool first = true;
  assert(first);
  if (first)  first = false;

  SDL_Init(SDL_INIT_VIDEO);

  _window = SDL_CreateWindow(title.data(), width, height, SDL_WINDOW_VULKAN    |
                                                          SDL_WINDOW_RESIZABLE);
  throw_if(!_window, "failed to create window!");
}

void Window::destroy() const
{
  SDL_DestroyWindow(_window);
  SDL_Quit();
}

auto Window::create_surface(VkInstance instance) const -> VkSurfaceKHR
{
  VkSurfaceKHR surface;
  throw_if(!SDL_Vulkan_CreateSurface(_window, instance, nullptr, &surface),
           "failed to create surface");
  return surface;
}

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
