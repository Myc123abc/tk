#pragma once

#include "window.hpp"

#include <thread>
#include <latch>
#include <unordered_map>
#include <queue>

namespace tk { namespace renderer {

class WindowManager
{
  friend class Window;
private:
  WindowManager()                                = default;
  ~WindowManager()                               = default;
public:
  WindowManager(WindowManager const&)            = delete;
  WindowManager(WindowManager&&)                 = delete;
  WindowManager& operator=(WindowManager const&) = delete;
  WindowManager& operator=(WindowManager&&)      = delete;

  static auto instance() noexcept -> WindowManager&
  {
    static WindowManager instance;
    return instance;
  }

  enum class Message
  {
    destroy = WM_APP,
    create_window,
  };

  void init() noexcept;

  void destroy() noexcept;

  static LRESULT CALLBACK wnd_proc(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param) noexcept;

  void create_window(int x, int y, uint32_t width, uint32_t height) noexcept;

  void message_process(HWND handle, Message msg, WPARAM w_param, LPARAM l_param) noexcept;

  void msg_destroy() noexcept;

  void msg_create_window(int x, int y, uint32_t width, uint32_t height) noexcept;

  void close_window(HWND handle) noexcept;

private:
  static constexpr wchar_t Fullscreen_Class[] = L"vn::window::WindowManager::Fullscreen";
  static constexpr wchar_t Window_Class[]     = L"vn::window::WindowManager::Window";

private:
  std::jthread                     _thread;
  DWORD                            _thread_id{};
  std::latch                       _message_queue_create_complete{ 1 };
  std::unordered_map<HWND, Window> _windows;
};

inline static auto& g_wnd_mgr{ WindowManager::instance() };

}}
