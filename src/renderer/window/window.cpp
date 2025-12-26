#include "window.hpp"
#include "../renderer.hpp"
#include "window_manager.hpp"

namespace tk { namespace renderer {

void Window::init(int x, int y, uint32_t width, uint32_t height) noexcept
{
  // _handle = CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP, Window_Class, nullptr, WS_POPUP | WS_MINIMIZEBOX,
  _handle = CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP, WindowManager::Window_Class, nullptr, WS_OVERLAPPEDWINDOW,
    x, y, width, height, 0, 0, GetModuleHandleW(nullptr), 0);
  err_if(!_handle, "failed to create window");

  // create window render resource
  g_renderer.send_message(Renderer::Message_Window_Create{ _handle, width, height });

  ShowWindow(_handle, SW_SHOW); // TODO: show after first frame render finish and also include some images uploaded finish
}

void Window::destroy() const noexcept
{
  ShowWindow(_handle, SW_HIDE);
  g_renderer.send_message(Renderer::Message_Window_Destroy{ _handle });
}

}}
