//
// graphics engine
//

#pragma once

#include "../Window.hpp"
#include "../DestructorStack.hpp"
#include "MemoryAllocator.hpp"
#include "CommandPool.hpp"
#include "Device.hpp"

#include <glm/glm.hpp>
#include <SDL3/SDL_events.h>

namespace tk { namespace graphics_engine {

  class GraphicsEngine
  {
  public:
    GraphicsEngine()  = default;
    ~GraphicsEngine() = default;

    GraphicsEngine(GraphicsEngine const&)            = delete;
    GraphicsEngine(GraphicsEngine&&)                 = delete;
    GraphicsEngine& operator=(GraphicsEngine const&) = delete;
    GraphicsEngine& operator=(GraphicsEngine&&)      = delete;

    // HACK: expand to multi windows
    /**
     * initialize graphics engine
     * need a main window (while vulkan can use offscreen rendering)
     * @param window main window
     * @throw std::runtime_error failed to init
     */
    void init(Window& window);

    void destroy();

    //
    // run
    //
    void resize_swapchain();

    void render_begin();
    void update();
    void render();
    void render_end();

  private:
    //
    // initialize resources
    //
    void create_instance();
    void create_debug_messenger();
    void create_surface();
    void select_physical_device();
    void create_device_and_get_queues();
    void create_swapchain(VkSwapchainKHR old_swapchain = VK_NULL_HANDLE);
    void init_command_pool();
    void init_memory_allocator();
    void create_frame_resources();

    void create_sdf_rendering_resource();

    // vk extension funcs
    void load_instance_extension_funcs();
    void load_device_extension_funcs();

    // rendering
    void set_pipeline_state(Command const& cmd);

    struct FrameResource;
    auto frame_begin() -> FrameResource*; 
    void frame_end();

  private:
    //
    // common resources
    //
    // HACK: expand to multi-windows manage, use WindowManager in future.
    Window*                      _window          = nullptr;
    VkInstance                   _instance        = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT     _debug_messenger = VK_NULL_HANDLE;
    VkSurfaceKHR                 _surface         = VK_NULL_HANDLE;
    VkPhysicalDevice             _physical_device = VK_NULL_HANDLE;
    Device                       _device;
    VkQueue                      _graphics_queue  = VK_NULL_HANDLE;
    VkQueue                      _present_queue   = VK_NULL_HANDLE;
    VkSwapchainKHR               _swapchain       = VK_NULL_HANDLE;
    std::vector<Image>           _swapchain_images;
    CommandPool                  _command_pool;
    MemoryAllocator              _mem_alloc;
    DestructorStack              _destructors;

    //
    // frame resources
    //
    struct FrameResource
    {
      Command     cmd;
      VkFence     fence                 = VK_NULL_HANDLE;
      VkSemaphore image_available_sem   = VK_NULL_HANDLE; 
      VkSemaphore render_finished_sem   = VK_NULL_HANDLE;
    };
    std::vector<FrameResource> _frames;
    uint32_t                   _current_frame                 = {};
    uint32_t                   _current_swapchain_image_index = {};
    auto get_current_frame()           noexcept -> FrameResource& { return _frames[_current_frame]; }
    auto get_current_swapchain_image() noexcept -> Image&         { return _swapchain_images[_current_swapchain_image_index]; }

    //
    // SDF rendering resources
    //
    Shader         _sdf_vert;
    Shader         _sdf_frag;
    PipelineLayout _sdf_pipeline_layout;
    void render_sdf();

    static constexpr uint32_t Buffer_Size = 1024;
    Buffer _buffer;
    void create_buffer();

    struct PushConstant_SDF
    {
      VkDeviceAddress address = {};
      uint32_t        offset  = {}; // offset of shape infos
      uint32_t        num     = {}; // number of shape infos
    };
    struct ShapeInfo
    {
      uint32_t  offset = {}; // offset of points
      uint32_t  num    = {}; // number of points
      glm::vec4 color;
    };

    // FIXME: tmp
    std::vector<glm::vec2> points;
    std::vector<GraphicsEngine::ShapeInfo> shape_infos;
  };

}}