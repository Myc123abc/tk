#pragma once

#include "../resource/render_resource.hpp"
#include "../config.hpp"
#include "../../util/message_queue.hpp"
#include "../../ui/image_manager.hpp"

#include <rigtorp/SPSCQueue.h>

#include <atomic>
#include <thread>
#include <functional>
#include <deque>
#include <unordered_map>
#include <variant>
#include <unordered_set>

namespace tk::renderer {

Singleton(Renderer, g_renderer,
public:
  void init() noexcept;
  void destroy() noexcept;

  void message_process() noexcept;
  void add_frame_render_complete_func(std::move_only_function<void()>&& func) noexcept;

  void acquire_frame() noexcept { _frame_sem.acquire(); }

  struct RenderInfo
  {
    HWND           handle{};
    ui::FrameData* frame_data{};
    HWND           blur_host_window{};
    RECT           blur_window_rect{};
  };
  void submit(RenderInfo const& info) noexcept { _render_infos.emplace(info); }

  auto is_sleeping() const noexcept { return _render_infos.empty(); }
  void wakeup() noexcept { _cmds_empty.release(); }

private:
  void preprocess_render()  noexcept;
  void upload_images() noexcept;

  void render() noexcept;
  void render(RenderResource& res, ui::FrameData* frame_data) const noexcept;
  void generate_mipmap() noexcept;

  void postprocess_render() noexcept;

private:
  std::jthread                                _thread;
  std::atomic_bool                            _exit{};
  std::deque<std::move_only_function<bool()>> _frame_render_complete_funcs;
  std::unordered_map<HWND, RenderResource>    _res;
  std::unordered_set<HWND>                    _destroied_windows;
  std::vector<HWND>                           _render_windows;

  rigtorp::SPSCQueue<RenderInfo> _render_infos{ Render_Info_Queue_Capacity };
  std::binary_semaphore          _frame_sem{ 1 };
  std::binary_semaphore          _cmds_empty{ 0 };

  std::unordered_map<HWND, HWND> _show_blur_wnds;
  HWND                           _blur_host_window{};
  RECT                           _blur_window_rect{};
  uint32_t                       _present_frame_idx{};

////////////////////////////////////////////////////////////////////////////////
///                           Image
////////////////////////////////////////////////////////////////////////////////

private:
  std::unordered_map<ui::ImageHandle, Image>  _images;
  std::unordered_map<ui::ImageHandle, Bitmap> _bitmaps;
  std::unordered_map<ui::ImageHandle, Image>  _upload_images;
  std::vector<ui::ImageHandle>                _pending_mipmap_image_handles;

////////////////////////////////////////////////////////////////////////////////
///                              Message Process
////////////////////////////////////////////////////////////////////////////////

public:
  struct Message_Window_Create
  {
    HWND     handle{};
    uint32_t width{};
    uint32_t height{};
  };

  struct Message_Window_Destroy
  {
    HWND handle{};
    HWND blur_handle{};
  };

  struct Message_Window_Update
  {
    HWND     handle{};
    uint32_t width{};
    uint32_t height{};
  };

  struct Message_Show_Blur_Window
  {
    HWND handle{};
    HWND blur_handle{};
  };

  using Message = std::variant<
    Message_Window_Create,
    Message_Window_Destroy,
    Message_Window_Update,
    Message_Show_Blur_Window
  >;

  void send_message(Message&& msg) noexcept
  {
    _msg_queue.send(std::move(msg));
  }

  struct Message_Upload_Image
  {
    Bitmap          bitmap;
    ui::ImageHandle handle;
    bool            use_mipmap{};
  };

  struct Message_Create_Image
  {
    ui::ImageHandle handle{};
    uint32_t        width{};
    uint32_t        height{};
    ImageFormat     format{};
  };

  struct Message_Destroy_Image
  {
    ui::ImageHandle handle{};
  };

  using UIContextMessage = std::variant<
    Message_Upload_Image,
    Message_Create_Image,
    Message_Destroy_Image
  >;

  void send_message(UIContextMessage&& msg) noexcept
  {
    _ui_ctx_msg_queue.send(std::move(msg));
  }

private:
  struct MessageHandler
  {
    Renderer& renderer;
    void operator()(Message_Window_Create const& msg) const noexcept;
    void operator()(Message_Window_Update const& msg) const noexcept;
    void operator()(Message_Window_Destroy const& msg) const noexcept;
    void operator()(Message_Show_Blur_Window const& msg) const noexcept;
  };
  struct UIContextMessageHandler
  {
    Renderer& renderer;
    void operator()(Message_Upload_Image const& msg) const noexcept;
    void operator()(Message_Destroy_Image const& msg) const noexcept;
    void operator()(Message_Create_Image const& msg) const noexcept;
  };
  MessageQueue<Message, Renderer_Msg_Queue_Capacity>          _msg_queue;
  MessageQueue<UIContextMessage, Renderer_Msg_Queue_Capacity> _ui_ctx_msg_queue;
)

}
