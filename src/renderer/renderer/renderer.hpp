#pragma once

#include "../resource/render_resource.hpp"
#include "../../ui/image_manager.hpp"

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

  void create_window_resource(HWND handle, uint32_t width, uint32_t height) noexcept;
  void destroy_window_resource(HWND handle, HWND blur_handle) noexcept;
  void resize_window_resource(HWND handle, uint32_t width, uint32_t height) noexcept;
  void show_blur_window(HWND handle, HWND blur_handle) noexcept { _show_blur_wnds.emplace(handle, blur_handle); }

  void create_image(ui::ImageHandle handle, uint32_t width, uint32_t height, ImageFormat format) noexcept;
  void destroy_image(ui::ImageHandle handle) noexcept;
  void upload_image(ui::ImageHandle handle, uint32_t width, uint32_t height, Bitmap const& bitmap, bool use_mipmap = false) noexcept;

  void clear_blur_resize_data() noexcept { _blur_host_window = {}; _blur_window_rect = {}; }

  auto descriptor_idx(ui::ImageHandle handle) noexcept { return _images[handle].srv().index(); }

private:
  void preprocess_render()  noexcept;
  void upload_images() noexcept;

  void process_render() noexcept;
  void render(RenderResource& res, ui::FrameData const* frame_data) noexcept;
  void generate_mipmap() noexcept;

  void postprocess_render() noexcept;

private:
  std::deque<std::move_only_function<bool()>> _frame_render_complete_funcs;
  std::unordered_map<HWND, RenderResource>    _res;
  std::unordered_set<HWND>                    _destroied_windows;
  std::vector<HWND>                           _render_windows;

  std::vector<RenderInfo> _render_infos;

  std::unordered_map<HWND, HWND> _show_blur_wnds;
  HWND                           _blur_host_window{};
  Rect                           _blur_window_rect{};

////////////////////////////////////////////////////////////////////////////////
///                           Image
////////////////////////////////////////////////////////////////////////////////

  std::unordered_map<ui::ImageHandle, Image>  _images;
  std::unordered_map<ui::ImageHandle, Bitmap> _bitmaps;
  std::unordered_map<ui::ImageHandle, Image>  _upload_images;
  std::vector<ui::ImageHandle>                _pending_mipmap_image_handles;
)

}
