#pragma once

#include "tk/type.hpp"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <vector>
#include <string_view>
#include <functional>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#error only support win32 now
#endif

namespace tk {

class Window
{
public:
  void init(std::string_view title, uint32_t width, uint32_t height);

  void destroy() const noexcept;

  static auto get_vulkan_instance_extensions() noexcept -> std::vector<char const*>;

  auto create_vulkan_surface(VkInstance instance) const noexcept -> VkSurfaceKHR;

  void show() noexcept;

  auto get_framebuffer_size() const noexcept -> glm::vec2;

  void event_process() const noexcept;

  void set_resize_swapchain(std::function<void()> const& f)              noexcept { resize_swapchain         = f; }
  void set_get_swapchain_image_size(std::function<glm::vec2()> const& f) noexcept { get_swapchain_image_size = f; }

  auto state() const noexcept { return _state; }

  auto get_mouse_position() const noexcept -> glm::vec2;

#ifdef _WIN32
private:
  static void CALLBACK message_process(LPVOID) noexcept;
  inline static LPVOID _main_fiber{};
  inline static LPVOID _message_fiber{};

private:
  friend LRESULT WINAPI window_process_callback(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param);

  LPCWSTR      ClassName{ L"main window" };
  HWND         _handle{};
  type::window _state{ type::window::suspended };
  std::function<void()>      resize_swapchain; // TODO: resize swapchain should be in single window? a window a swapchain? is right?
  std::function<glm::vec2()> get_swapchain_image_size;
#endif
};

}