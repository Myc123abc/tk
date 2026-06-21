#pragma once

#include "resource/render_resource.hpp"
#include "util/rect.hpp"

#include <functional>
#include <deque>
#include <unordered_map>
#include <unordered_set>

namespace tk::renderer {

Singleton(Renderer, g_renderer,
public:
  void init() noexcept;
  void destroy() noexcept;

  void render() noexcept;
  void wait_idle() noexcept;

  void message_process() noexcept;
  void add_frame_render_complete_func(std::move_only_function<void()>&& func) noexcept;

  struct RenderInfo
  {
    HWND           handle{};
    ui::FrameData* frame_data{};
    HWND           blur_host_window{};
    Rect           blur_window_rect{};
  };
  void submit(RenderInfo const& info) noexcept { _render_infos.emplace_back(info); }

  void create_window_resource(HWND handle, uint width, uint height) noexcept;
  void destroy_window_resource(HWND handle, HWND blur_handle) noexcept;
  void resize_window_resource(HWND handle, uint width, uint height) noexcept;
  void show_blur_window(HWND handle, HWND blur_handle) noexcept { _show_blur_wnds.emplace(handle, blur_handle); }

  void clear_blur_resize_data() noexcept { _blur_host_window = {}; _blur_window_rect = {}; }

private:
  void init_images() noexcept;
  void destroy_images() noexcept;

  void preprocess_render()  noexcept;

  void process_render() noexcept;
  void render(RenderResource& res, ui::FrameData const* frame_data) noexcept;

  void postprocess_render() noexcept;
  void push_tmp_frame_render_complete_funcs() noexcept;

private:
  std::deque<std::move_only_function<bool()>> _frame_render_complete_funcs;
  std::deque<std::move_only_function<void()>> _tmp_frame_render_complete_funcs;
  std::unordered_map<HWND, RenderResource>    _res;
  std::unordered_set<HWND>                    _destroied_windows;
  std::vector<HWND>                           _render_windows;

  std::vector<RenderInfo> _render_infos;

  std::unordered_map<HWND, HWND> _show_blur_wnds;
  HWND                           _blur_host_window{};
  Rect                           _blur_window_rect{};

  ImageHandle _discard_image;
  ImageHandle _composite_image;
)

}
