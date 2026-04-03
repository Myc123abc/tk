#pragma once

#include "../resource/render_data.hpp"
#include "../resource/render_resource.hpp"
#include "../config.hpp"
#include "../../util/message_queue.hpp"
#include "../../ui/command_list.hpp"
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
    HWND             handle{};
    ui::CommandList* cmd{};
    HWND             blur_host_window{};
    RECT             blur_window_rect{};
  };
  void submit(RenderInfo const& info) noexcept { _cmds.emplace(info); }

  auto is_sleeping() const noexcept { return _cmds.empty(); }
  void wakeup() noexcept { _cmds_empty.release(); }

private:
  void preprocess_render()  noexcept;
  void upload_images() noexcept;
  void generate_render_data(HWND handle, ui::CommandList* cmd) noexcept;

  void render() noexcept;
  void generate_mipmap() noexcept;

  void postprocess_render() noexcept;

private:
  std::jthread                                _thread;
  std::atomic_bool                            _exit{};
  std::deque<std::move_only_function<bool()>> _frame_render_complete_funcs;
  std::unordered_map<HWND, RenderResource>    _res;
  std::unordered_set<HWND>                    _destroied_windows;

  rigtorp::SPSCQueue<RenderInfo> _cmds{ Command_List_Queue_Capacity };
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
///                          Render Data
////////////////////////////////////////////////////////////////////////////////

private:
  void add_vertices_indices(RenderData& render_data, std::pair<glm::vec2, glm::vec2> bounding_rectangle) noexcept;
  void add_shape_property(RenderData& render_data, ShapeProperty::Type type, glm::vec4 color, float thickness, std::vector<float> const& values) noexcept;
  void add_shape(RenderData& render_data, ShapeProperty::Type type, glm::vec4 color, float thickness, std::vector<float> const& values, std::pair<glm::vec2, glm::vec2> bounding_rectangle) noexcept;

private:
  std::unordered_map<HWND, RenderData> _render_datas;
  std::vector<HWND>                    _render_windows;

  struct OperatorShapeRenderData
  {
    renderer::ShapeProperty::Operator op;
    std::vector<glm::vec2>            points;
    uint32_t                          offset{};
  } _op_data;

  uint32_t                 _shape_properties_offset{};
  uint16_t                 _idx_beg{};
  std::optional<glm::vec4> _tmp_color;
  std::vector<float>       _path_data;
  std::vector<glm::vec2>   _path_points;

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
