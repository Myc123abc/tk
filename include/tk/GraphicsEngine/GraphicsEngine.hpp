//
// graphics engine
//

#pragma once

#include "../DestructorStack.hpp"
#include "TextEngine/TextEngine.hpp"
#include "FrameResources.hpp"
#include "Pipeline/GraphicsPipeline.hpp"

#include <span>

namespace tk { namespace graphics_engine {

  // INFO: when change header fields, remebering also change header_field_count and emplace field data to device address in sdf_render function
  struct ShapeProperty
  {
    inline static constexpr uint32_t header_field_count{ 7 };

    type::Shape   type{};
    glm::vec4     color{};
    uint32_t      thickness{};
    type::ShapeOp op{};
    
    std::vector<float> values;
  };

  class GraphicsEngine
  {
  public:
    GraphicsEngine()            = default;
    GraphicsEngine(auto const&) = delete;
    GraphicsEngine(auto&&)      = delete;
    auto operator=(auto const&) = delete;
    auto operator=(auto&&)      = delete;

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

    auto parse_text(std::string_view text, glm::vec2 pos, float size, type::FontStyle style, std::vector<Vertex>& vertices, std::vector<uint16_t>& indices, uint32_t offset, uint16_t& idx) -> glm::vec2;

    void sdf_render_begin();
    void sdf_render(std::span<Vertex> vertices, std::span<uint16_t> indices, std::span<ShapeProperty> shape_properties);

    void wait_device_complete() const noexcept { vkDeviceWaitIdle(_device); }

    void load_fonts(std::vector<std::string_view> const& fonts);

  private:

    //
    // initialize resources
    //
    void create_instance();
    void create_debug_messenger();
    void create_surface();
    void select_physical_device();
    void create_device_and_get_queues();
    void create_swapchain();
    void init_command_pool();
    void init_memory_allocator();
    void create_frame_resources();
    void create_sampler();

    // vk extension funcs
    void load_instance_extension_funcs();
    void load_device_extension_funcs();

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
    VkDevice                     _device{};
    VkQueue                      _graphics_queue{};
    VkQueue                      _present_queue{};
    Swapchain                    _swapchain;
    CommandPool                  _command_pool;
    MemoryAllocator              _mem_alloc;
    DestructorStack              _destructors;

    bool _wait_fence{ true };

    FrameResources _frames;

    //
    // SDF rendering resources
    //
    void render_sdf();
    void init_sdf_resources();

    struct PushConstant_SDF
    {
      VkDeviceAddress vertices{};
      VkDeviceAddress shape_properties{};
      glm::vec2       window_extent{};
    };

    FramesDynamicBuffer _sdf_buffer;
    GraphicsPipeline    _sdf_graphics_pipeline;

    //
    // Text Rendering
    //
    VkSampler  _sampler{};
    TextEngine _text_engine;
  };
}}