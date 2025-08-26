//
// graphics engine
//

#pragma once

#include "../Window.hpp"
#include "../DestructorStack.hpp"
#include "MemoryAllocator.hpp"
#include "CommandPool.hpp"
#include "Device.hpp"
#include "tk/type.hpp"
#include "TextEngine/TextEngine.hpp"

#include <span>

namespace tk { namespace graphics_engine {

  // INFO: when change header fields, remebering also change header_field_count and emplace field data to device address in sdf_render function
  struct ShapeProperty
  {
    inline static constexpr uint32_t header_field_count{ 7 };

    type::shape        type{};
    glm::vec4          color{};
    uint32_t           thickness{};
    type::shape_op     op{};
    
    std::vector<float> values;
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

    void wait_fence(bool b) noexcept { _wait_fence = b; }

    //
    // run
    //
    void resize_swapchain();
    auto get_swapchain_image_size() -> glm::vec2;
    
    auto frame_begin() -> bool;
    void frame_end();

    void render_end();

    auto parse_text(std::string_view text, glm::vec2 pos, float size, float italic_factor, std::vector<Vertex>& vertices, std::vector<uint16_t>& indices, uint32_t offset, uint16_t& idx) -> std::pair<glm::vec2, glm::vec2>;

    void sdf_render_begin();
    void sdf_render(std::span<Vertex> vertices, std::span<uint16_t> indices, std::span<ShapeProperty> shape_properties);

    void wait_device_complete() const noexcept { vkDeviceWaitIdle(_device); }

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
    // TODO: expand which not need color attachemtn, such as compute pipeline?
    void render_begin(Image& image);

    void init_text_engine();

    void init_gpu_resource();

  private:
    //
    // common resources
    //
    // HACK: expand to multi-windows manage, use WindowManager in future.
    Window*                      _window{};
    VkInstance                   _instance{};
    VkDebugUtilsMessengerEXT     _debug_messenger{};
    VkSurfaceKHR                 _surface{};
    VkPhysicalDevice             _physical_device{};
    Device                       _device;
    VkQueue                      _graphics_queue{};
    VkQueue                      _present_queue{};
    VkSwapchainKHR               _swapchain{};
    std::vector<Image>           _swapchain_images;
    CommandPool                  _command_pool;
    MemoryAllocator              _mem_alloc;
    DestructorStack              _destructors;

    bool _wait_fence{ true };

    static constexpr auto    Vulkan_Version{ VK_API_VERSION_1_4 };
    std::vector<const char*> _device_extensions
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
      VkFence     fence{};
      VkSemaphore acquire_sem{};
      VkSemaphore submit_sem{};
    };
    std::vector<FrameResource> _frames;
    std::vector<VkSemaphore>   _submit_sems;
    uint32_t                   _current_frame{};
    uint32_t                   _current_swapchain_image_index{};
    auto get_current_frame()           noexcept -> FrameResource& { return _frames[_current_frame]; }
    auto get_current_swapchain_image() noexcept -> Image&         { return _swapchain_images[_current_swapchain_image_index]; }

    Buffer _descriptor_buffer;

    //
    // SDF rendering resources
    //
    RenderPipeline _sdf_render_pipeline;
    void render_sdf();

    static constexpr uint32_t Buffer_Size{ 1024 * 1024 };
    std::vector<Buffer> _buffers;
    void create_buffer();

    struct PushConstant_SDF
    {
      VkDeviceAddress vertices{};
      VkDeviceAddress shape_properties{};
      glm::vec2       window_extent{};
    };

    //
    // Text Rendering
    //
    VkSampler _sampler{};
    TextEngine _text_engine;
  };
}}