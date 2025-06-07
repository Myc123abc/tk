//
// graphics engine
//

#pragma once

#include "../Window.hpp"
#include "../DestructorStack.hpp"
#include "MemoryAllocator.hpp"
#include "CommandPool.hpp"
#include "Device.hpp"
#include "../type.hpp"

#include <glm/glm.hpp>
#include <SDL3/SDL_events.h>

#include <span>

// TODO: these should be delete after implement text rendering based on msdf-atlas-gen
#define FREETYPE_USE 0

namespace tk { namespace graphics_engine {

  struct ShapeInfo
  {
    type::shape    type      = {};
    uint32_t       offset    = {}; // offset of points
    uint32_t       num       = {}; // number of points
    glm::vec4      color;
    uint32_t       thickness = {};
    type::shape_op op        = {};
  };

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
    void update(std::span<glm::vec2> points, std::span<ShapeInfo> infos);
    void render(uint32_t offset, uint32_t num);
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
    void create_sampler();

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
    Window*                      _window          = {};
    VkInstance                   _instance        = {};
    VkDebugUtilsMessengerEXT     _debug_messenger = {};
    VkSurfaceKHR                 _surface         = {};
    VkPhysicalDevice             _physical_device = {};
    Device                       _device;
    VkQueue                      _graphics_queue  = {};
    VkQueue                      _present_queue   = {};
    VkSwapchainKHR               _swapchain       = {};
    std::vector<Image>           _swapchain_images;
    CommandPool                  _command_pool;
    MemoryAllocator              _mem_alloc;
    DestructorStack              _destructors;

    static constexpr auto    Vulkan_Version    = VK_API_VERSION_1_4;
    std::vector<const char*> Device_Extensions = 
    {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
      VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
    };

    //
    // frame resources
    //
    struct FrameResource
    {
      Command     cmd;
      VkFence     fence       = {};
      VkSemaphore acquire_sem = {};
      VkSemaphore submit_sem  = {};
    };
    std::vector<FrameResource> _frames;
    std::vector<VkSemaphore>   _submit_sems;
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

    static constexpr uint32_t Buffer_Size = 1024 * 1024;
    std::vector<Buffer> _buffers;
    void create_buffer();

    struct PushConstant_SDF
    {
      VkDeviceAddress address = {};
      uint32_t        offset  = {}; // offset of shape infos
      uint32_t        num     = {}; // number of shape infos
      glm::vec2       window_extent;
    };

    //
    // Text Render
    //
    VkSampler  _sampler = {};
    void load_font();
#if FREETYPE_USE
    FT_Library _ft_lib  = {};
    FT_Face    _ft_face = {};
    Image      _ft_image;
    Buffer     _descriptor_buffer;
    PipelineLayout   _text_render_pipeline_layout;
    DescriptorLayout _text_render_destriptor_layout;
    Shader           _text_render_vert, _text_render_frag;
    struct PushConstant_text_render
    {
      glm::vec4 color;
    };
#endif
  };

}}