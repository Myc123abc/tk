#include "tk/Window.hpp"
#include "tk/ErrorHandling.hpp"

#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#endif

namespace
{
#ifdef _WIN32

template <typename... Args>
inline void check(bool x, std::format_string<Args...> fmt, Args&&... args)
{
  tk::throw_if(!x, "[WIN32] {}: {}", std::format(fmt, std::forward<Args>(args)...), GetLastError());
}

auto to_wstring(std::string_view str) -> std::wstring
{
  int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), nullptr, 0);
  std::wstring wstr(size, 0);
  MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), &wstr[0], size);
  return wstr;
}

auto to_string(const wchar_t* wstr) -> std::string
{
  int size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
  std::string str(size - 1, 0);
  WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str.data(), size, nullptr, nullptr);
  return str;
}

#endif

}

namespace tk {

#ifdef _WIN32

LRESULT WINAPI window_process_callback(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param)
{
  using enum tk::type::window;

  auto window = reinterpret_cast<Window*>(GetWindowLongPtr(handle, GWLP_USERDATA));
  if (!window) return DefWindowProcW(handle, msg, w_param, l_param);

  switch (msg)
  {
  case WM_SIZE:
  {
    if (w_param == SIZE_MINIMIZED)
    {
      window->_state = suspended;
      return 0;
    }
    else if (w_param == SIZE_RESTORED)
      window->_state = running;

    if (!window->get_swapchain_image_size || !window->resize_swapchain)
      return 0;
    auto swapchain_image_size{ window->get_swapchain_image_size() };
    auto framebuffer_size{ window->get_framebuffer_size() };
    if (swapchain_image_size != framebuffer_size)
    {
      if (framebuffer_size.x == 0 || framebuffer_size.y == 0)
      {
        window->_state = suspended;
        return 0;
      }
      window->resize_swapchain();
      window->_state = running;
    }
    return 0;
  }

  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;

  case WM_SETCURSOR:
    if (LOWORD(l_param) == HTCLIENT)
    {
      SetCursor(LoadCursor(nullptr, IDC_ARROW));
      return TRUE;
    }
    break;
  }
  return DefWindowProcW(handle, msg, w_param, l_param);
}

void Window::init(std::string_view title, uint32_t width, uint32_t height)
{
  // adjust width and height as client area
  RECT rect{ 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
  check(AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0), "failed to adjust window rectangle");

  // register class
  WNDCLASSEXW wc{ sizeof(wc), CS_CLASSDC, window_process_callback, 0, 0, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, ClassName, nullptr };
  check(RegisterClassExW(&wc), "failed to register class {}", to_string(ClassName));

  // create window
  _handle = ::CreateWindowW(wc.lpszClassName, to_wstring(title).c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, wc.hInstance, nullptr);
  check(_handle, "failed to create window");

  // set pointer in window
  check(!SetWindowLongPtrW(_handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR >(this)), "failed to set user pointer");
}

void Window::destroy() const noexcept
{
  assert(_handle);
  check(!DestroyWindow(_handle), "failed to destroy window");
  check(UnregisterClassW(ClassName, GetModuleHandle(nullptr)), "failed to unregister class {}", to_string(ClassName));
}

auto Window::get_vulkan_instance_extensions() noexcept -> std::vector<char const*>
{
  return { "VK_KHR_surface", "VK_KHR_win32_surface" };
}

auto Window::create_vulkan_surface(VkInstance instance) const noexcept -> VkSurfaceKHR
{
  VkSurfaceKHR surface{};
  VkWin32SurfaceCreateInfoKHR info
  {
    .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
    .hinstance = GetModuleHandle(nullptr),
    .hwnd      = _handle,
  };
  throw_if(vkCreateWin32SurfaceKHR(instance, &info, nullptr, &surface) != VK_SUCCESS,
           "failed to create surface");
  return surface;
}

void Window::show() noexcept
{
  ShowWindow(_handle, SW_SHOWDEFAULT);
  _state = type::window::running;
}

auto Window::get_framebuffer_size() const noexcept -> glm::vec2
{
  RECT rect{};
  check(GetClientRect(_handle, &rect), "failed to get client rectangle");
  return { rect.right - rect.left, rect.bottom - rect.top };
}

auto Window::event_process() const noexcept -> type::window
{
  MSG msg{};
  while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
  {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
    if (msg.message == WM_QUIT)
      return type::window::closed;
  }
  return type::window::running;
}

#endif

}