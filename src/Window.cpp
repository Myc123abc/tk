#include "tk/Window.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/GraphicsEngine/GraphicsEngine.hpp"

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

constexpr uint32_t Move_Timer{ 0 };

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
    if (window->_engine == nullptr)
      break;

    if (w_param == SIZE_MINIMIZED)
    {
      window->_state = suspended;
      return 0;
    }
    else if (w_param == SIZE_RESTORED)
      window->_state = running;

    auto swapchain_image_size{ window->_engine->get_swapchain_image_size() };
    auto framebuffer_size{ window->get_framebuffer_size() };
    if (swapchain_image_size != framebuffer_size)
    {
      if (framebuffer_size.x == 0 || framebuffer_size.y == 0)
      {
        window->_state = suspended;
        return 0;
      }
      window->_engine->resize_swapchain();
      window->_state = running;
      SwitchToFiber(Window::_main_fiber);
    }
  }
  return 0;

  case WM_CLOSE:
  {
    window->_state = closed;
  }
  return 0;

  case WM_SETCURSOR:
  {
    if (LOWORD(l_param) == HTCLIENT)
    {
      SetCursor(LoadCursor(nullptr, IDC_ARROW));
      return TRUE;
    }
  }
  break;

  case WM_ENTERSIZEMOVE:
  {
    SetTimer(handle, Move_Timer, 1, nullptr);
    window->_engine->wait_fence(false);
  }
  break;

  case WM_EXITSIZEMOVE:
  {
    KillTimer(handle, Move_Timer);
    window->_engine->wait_fence(true);
  }
  break;

  case WM_TIMER:
  {
    if (w_param == Move_Timer)
      SwitchToFiber(Window::_main_fiber);
  }
  break;

  }
  return DefWindowProcW(handle, msg, w_param, l_param);
}

void Window::init_keys() noexcept
{
  using enum type::key;

  _keys =
  {
    { q,     {} },
    { space, {} },
  };
}

void Window::init(std::string_view title, uint32_t width, uint32_t height)
{
  static bool first{ true };
  assert(first);
  if (first) first = false;
  // TODO: unprocess for multiple windows
  _main_fiber = ConvertThreadToFiber(nullptr);
  check(_main_fiber, "failed to convert thread to fiber");
  _message_fiber = CreateFiber(0, &message_process, nullptr);
  check(_message_fiber, "failed to create message fiber");

  // register class
  WNDCLASSEXW wc{ sizeof(wc), CS_OWNDC | CS_VREDRAW | CS_HREDRAW, window_process_callback, 0, 0, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, ClassName, nullptr };
  check(RegisterClassExW(&wc), "failed to register class {}", to_string(ClassName));

  // adjust width and height as client extent
  RECT rect{ 0, 0, static_cast<int>(width), static_cast<int>(height) };
  check(AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE), "failed to adjust window rectangle");
  width  = rect.right  - rect.left;
  height = rect.bottom - rect.top;
  
  // put window in center of screen
  auto x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
  auto y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

  // create window
  _handle = ::CreateWindowW(wc.lpszClassName, to_wstring(title).c_str(), WS_OVERLAPPEDWINDOW, x, y, width, height, nullptr, nullptr, wc.hInstance, nullptr);
  check(_handle, "failed to create window");

  // set pointer in window
  check(!SetWindowLongPtrW(_handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR >(this)), "failed to set user pointer");

  init_keys();
}

void Window::destroy() const
{
  assert(_handle);
  DeleteFiber(_message_fiber);
  check(ConvertFiberToThread(), "failed to convert fiber to thread");
  check(DestroyWindow(_handle), "failed to destroy window");
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

void Window::event_process() const noexcept
{
  SwitchToFiber(_message_fiber);
}

auto Window::get_mouse_position() const noexcept -> glm::vec2
{
  POINT pos{};
  GetCursorPos(&pos);
  ScreenToClient(_handle, &pos);
  return { pos.x, pos.y };
}

void CALLBACK Window::message_process(LPVOID) noexcept
{
  MSG msg{};
  while (true)
  {
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
    SwitchToFiber(_main_fiber);
  }
}

auto to_vk(type::key k) -> int
{
  using enum type::key;
  switch (k)
  {
  case q:     return 'Q';
  case space: return VK_SPACE;
  }
  throw_if(true, "not have this key");
}

auto Window::get_key(type::key k) noexcept -> type::key_state
{
  using enum type::key;
  using enum type::key_state;

  auto  key_state = GetKeyState(to_vk(k)) & 0x8000;
  auto& key       = _keys[k];
  auto  now       = std::chrono::high_resolution_clock::now();

  if (key.state == release)
  {
    if (key_state == 0)
      return release;
    else
    {
      key.state      = press;
      key.start_time = now;
      key.last_time  = now;
      return press;
    }
  }
  else
  {
    if (key_state == 0)
    {
      key.state = release;
      return release;
    }
    else
    {
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - key.start_time).count();
      if (duration < _key_start_repeat_time)
      {
        return repeate_wait;
      }
      duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - key.last_time).count();
      if (duration > _key_repeat_interval)
      {
        key.last_time = now;
        return press;
      }
    }
  }
  assert(true);
  return {};
}

#endif

}