#pragma once

#include "tk/type.hpp"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <vector>
#include <string_view>

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

  void show() const noexcept;

  auto get_framebuffer_size() const noexcept -> glm::vec2;

  auto event_process() const noexcept -> type::window;

  auto is_resized() const noexcept { return _is_resized; }

private:
#ifdef _WIN32
  friend LRESULT WINAPI window_process_callback(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param);

  LPCWSTR ClassName{ L"main window" };
  HWND    _handle{};
  bool    _is_resized{};
#endif
};

}