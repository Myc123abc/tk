#include "tk/Window.hpp"
#include "tk/ErrorHandling.hpp"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_hints.h>

#include <cassert>

#if GET_DPI_IMPL
#ifdef _WIN32
#include <windows.h>
#else

#endif
#endif

namespace
{

inline void check(bool b, std::string_view msg)
{
  tk::throw_if(!b, "{}: {}", msg, SDL_GetError());
}

}

namespace tk {

auto Window::init(std::string_view title, uint32_t width, uint32_t height) -> Window&
{
  // HACK: expand to multi-windows manage
  static bool first = true;
  assert(first);
  if (first)  first = false;

  assert(width > 0 && height > 0);

  check(SDL_Init(SDL_INIT_VIDEO), "failed to init SDL");

  // Set SDL hint to receive mouse click events on window focus, otherwise SDL doesn't emit the event.
  // Without this, when clicking to gain focus, our widgets wouldn't activate even though they showed as hovered.
  // (This is unfortunately a global SDL setting, so enabling it might have a side-effect on your application.
  // It is unlikely to make a difference, but if your app absolutely needs to ignore the initial on-focus click:
  // you can ignore SDL_EVENT_MOUSE_BUTTON_DOWN events coming right after a SDL_EVENT_WINDOW_FOCUS_GAINED)
  check(SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1"), "failed to hint mouse focus clickthrough in sdl");
  
  // Disable auto-capture, this is preventing drag and drop across multiple windows
  check(SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "0"), "failed to disable mouse auto capture in sdl");

  // see https://github.com/libsdl-org/SDL/issues/6659
  check(SDL_SetHint("SDL_BORDERLESS_WINDOWED_STYLE", "0"), "failed to disable borderless window style in sdl");

  auto flags = SDL_WINDOW_VULKAN             |
               SDL_WINDOW_RESIZABLE          |
               SDL_WINDOW_HIGH_PIXEL_DENSITY |
               SDL_WINDOW_HIDDEN;
  _window = SDL_CreateWindow(title.data(), width, height, flags);
  check(_window, "failed to create window");

  return *this;
}

void Window::destroy() const
{
  SDL_DestroyWindow(_window);
}

auto Window::show() -> Window&
{
  check(SDL_SetWindowPosition(_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED), "failed to set position of window");
  check(SDL_ShowWindow(_window), "failed to show window");
  return *this;
}

auto Window::create_surface(VkInstance instance) const -> VkSurfaceKHR
{
  VkSurfaceKHR surface;
  check(SDL_Vulkan_CreateSurface(_window, instance, nullptr, &surface), "failed to create vulkan surface");
  return surface;
}

void Window::get_framebuffer_size(uint32_t& width, uint32_t& height) const
{
  check(SDL_GetWindowSizeInPixels(_window, (int*)&width, (int*)&height), "failed to get pixel size of window");
}

void Window::get_screen_size(uint32_t& width, uint32_t& height) const
{
  auto id   = SDL_GetDisplayForWindow(_window);
  check(id, "failed to get id of window");
  auto mode = SDL_GetCurrentDisplayMode(id);
  check(mode, "failed to get display mode of window");
  width  = mode->w;
  height = mode->h;
}
    
auto Window::get_vulkan_instance_extensions() -> std::vector<const char*>
{
  uint32_t count;
  auto ret = SDL_Vulkan_GetInstanceExtensions(&count);
  check(ret, "failed to get instance extensions of vulkan");
  return std::vector(ret, ret + count);
}

#if GET_DPI_IMPL
auto Window::get_dpi() const noexcept -> uint32_t
{
  uint32_t dpi{};

  auto properties = SDL_GetWindowProperties(_window);
  check(properties, "failed to get properties of window");

#ifdef _WIN32  
  auto hwnd = reinterpret_cast<HWND>(SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
  check(hwnd, "failed to get win32 HWND");
  dpi = GetDpiForWindow(hwnd);
  throw_if(dpi == 0, "failed to get dpi by GetDpiFromWindow(HWND hwnd)");
#else
  throw_if("TODO: expand other platform window handle. This message from Window::get_dpi()");
#endif

  return dpi;
}
#endif

}