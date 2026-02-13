#pragma once

#include "resource/render_data.hpp"
#include "resource/render_resource.hpp"
#include "pipeline.hpp"
#include "config.hpp"
#include "../util/message_queue.hpp"

#include <rigtorp/SPSCQueue.h>

#include <atomic>
#include <thread>
#include <functional>
#include <deque>
#include <unordered_map>
#include <variant>
#include <unordered_set>

namespace tk { namespace renderer {

class Renderer
{
private:
  Renderer()                           = default;
  ~Renderer()                          = default;
public:
  Renderer(Renderer const&)            = delete;
  Renderer(Renderer&&)                 = delete;
  Renderer& operator=(Renderer const&) = delete;
  Renderer& operator=(Renderer&&)      = delete;

  static auto instance() noexcept -> Renderer&
  {
    static Renderer instance;
    return instance;
  }

  void init() noexcept;
  void destroy() noexcept;

  void message_process() noexcept;
  void add_frame_render_complete_func(std::move_only_function<void()>&& func) noexcept;

  void acquire_frame() noexcept { _frame_sem.acquire(); }

  void render(HWND handle, RenderData* render_data) noexcept { _render_datas.emplace(handle, render_data); }

  auto is_sleeping() const noexcept { return _render_datas.empty(); }
  void wakeup() noexcept { _render_data_empty_sem.release(); }

private:
  void preprocess_render()  noexcept;
  void postprocess_render() noexcept;
  void render() noexcept;
  void render_sdf(RenderResource& res, RenderData* data) noexcept;
  void generate_mipmap() noexcept;

private:
  std::jthread                                     _thread;
  std::atomic_bool                                 _exit{};

  std::deque<std::move_only_function<bool()>>      _frame_render_complete_funcs;

  std::unordered_map<HWND, RenderResource>         _res;
  Pipeline                                         _sdf_pipeline;
  Pipeline                                         _mipmap_pipeline;

  std::unordered_set<HWND>                         _destroied_windows;
  std::vector<HWND>                                _rendered_windows;

  rigtorp::SPSCQueue<std::pair<HWND, RenderData*>> _render_datas{ Render_Data_Queue_Capacity };
  std::binary_semaphore                            _frame_sem{ 1 };
  std::binary_semaphore                            _render_data_empty_sem{ 0 };

  //
  // images
  //
public:
  auto get_image_indices() const noexcept { return _image_indices; }

private:
  std::unordered_map<uint32_t, Image>  _images;
  std::unordered_map<uint32_t, Bitmap> _bitmaps;
  std::unordered_map<uint32_t, Image>  _upload_images;
  std::vector<uint32_t>                _image_indices;
  std::vector<uint32_t>                _pending_mipmap_indices; 

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
  };

  struct Message_Window_Update
  {
    HWND     handle{};
    uint32_t width{};
    uint32_t height{};
  };

  using Message = std::variant<
    Message_Window_Create,
    Message_Window_Destroy,
    Message_Window_Update
  >;

  void send_message(Message&& msg) noexcept
  {
    _msg_queue.send(std::move(msg));
  }

  struct Message_Upload_Image
  {
    Bitmap   bitmap;
    uint32_t index{};
    bool     use_mipmap{};
  };

  struct Message_Create_Image
  {
    uint32_t    width{};
    uint32_t    height{};
    ImageFormat format{};
    uint32_t    index{};
  };

  struct Message_Destroy_Image
  {
    uint32_t index{};
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
};

inline static auto& g_renderer{ Renderer::instance() };

}}
