//
// window class
//
// TODO:
// currenly, only implement win32, also need to implement wayland, android
//
// in win32, use fiber to avoid modal loop blocking main loop when move and resize.
//

#pragma once

#include "tk/type.hpp"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <vector>
#include <string_view>
#include <unordered_map>
#include <chrono>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#error only support win32 now
#endif

namespace tk {

namespace graphics_engine { class GraphicsEngine; }

class Window
{
public:
  void init(std::string_view title, uint32_t width, uint32_t height);
  void set_engine(graphics_engine::GraphicsEngine* engine) noexcept { _engine = engine; }

  void destroy() const;

  static auto get_vulkan_instance_extensions() noexcept -> std::vector<char const*>;

  auto create_vulkan_surface(VkInstance instance) const noexcept -> VkSurfaceKHR;

  void show() noexcept;

  auto get_framebuffer_size() const noexcept -> glm::vec2;

  void event_process() const noexcept;

  auto state() const noexcept { return _state; }

  auto get_mouse_position() const noexcept -> glm::vec2;
  auto get_mouse_state() const noexcept -> type::mouse_state;

  void init_keys() noexcept;
  auto get_key(type::key k) noexcept -> type::key_state;

#ifdef _WIN32
private:
  // TODO: cpp coroutine will get better performance than win32 fiber,
  //       and maybe have std::fiber in future
  static void CALLBACK message_process(LPVOID) noexcept;
  inline static LPVOID _main_fiber{};
  inline static LPVOID _message_fiber{};

private:
  friend LRESULT WINAPI window_process_callback(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param);

  LPCWSTR      ClassName{ L"main window" };
  HWND         _handle{};
  type::window _state{ type::window::suspended };
#endif

private:
  graphics_engine::GraphicsEngine* _engine{};

  struct KeyState
  {
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point last_time;
    type::key_state state{ type::key_state::release };
  };
  uint32_t _key_start_repeat_time{ 400 };
  uint32_t _key_repeat_interval{ 80 };
  std::unordered_map<type::key, KeyState> _keys;
};

}