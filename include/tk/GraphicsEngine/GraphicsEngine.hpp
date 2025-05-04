//
// graphics engine
//
// use Vulkan to implement
//
// TODO:
// 1. use offscreen rendering, but when window size bigger than image size, it will be stretch,
//    maybe need to recreate image in this case.
// 2. use VkSampler as image sampler, like imgui_impl_vulkan.cpp:1031
// 3. embed spv
// 4. currently, not see mouse handling, just graphics handling
// 5. not handle font now
//
// TODO: today
// 1. the way to copy date t buffer and test a simple quard
//
// INFO:
// use MSAA and FXAA for AA
// about 2D pipeline, not use depth test, and vertex only store vec2 pos, uv, col
//


#pragma once

#include "../Window.hpp"
#include "../DestructorStack.hpp"
#include "MemoryAllocator.hpp"
#include "CommandPool.hpp"
#include "Shader.hpp"

#include <glm/glm.hpp>
#include <vk_mem_alloc.h>
#include <SDL3/SDL_events.h>

#include <vector>
#include <span>

namespace tk { namespace graphics_engine {

  // TODO: graphics engine only initialize.
  // you need add vertices, indices, uniform, and shaders to run it.
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

    struct IndexInfo
    {
      uint32_t offset  = {};
      uint32_t count   = {};
    };
    void render(std::span<IndexInfo> index_infos, glm::vec2 const& window_extent, glm::vec2 const& display_pos);

    void render_end();

    void update(std::span<Vertex> vertices, std::span<uint16_t> indices);

  private:
    //
    // initialize resources
    //
    void create_instance();
    void create_debug_messenger();
    void create_surface();
    void select_physical_device();
    void create_device_and_get_queues();
    void init_memory_allocator();
    void create_swapchain_and_rendering_image();
    void create_swapchain(VkSwapchainKHR old_swapchain = VK_NULL_HANDLE);
    void create_descriptor_set_layout();
    void create_compute_pipeline();
    void create_graphics_pipeline();
    void init_command_pool();
    void create_descriptor_pool();
    void create_descriptor_sets();
    void create_sync_objects();
    void create_frame_resources();
    void create_buffer();

    //
    // util 
    //
    static void transition_image_layout(VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout);
    static auto get_image_subresource_range(VkImageAspectFlags aspect) -> VkImageSubresourceRange;
    static void copy_image(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D src_extent, VkExtent2D dst_extent);

    // INFO: 
    // this is depth image memory barrier, depend on this link
    // https://stackoverflow.com/questions/62371266/why-is-a-single-depth-buffer-sufficient-for-this-vulkan-swapchain-render-loop
    // it's difficult for me now, so I just use multi depth images
    void depth_image_barrier_begin(VkCommandBuffer cmd);
    void depth_image_barrier_end(VkCommandBuffer cmd);

  private:
    //
    // common resources
    //
    // HACK: expand to multi-windows manage, use WindowManager in future.
    Window*                      _window                   = nullptr;
    VkInstance                   _instance                 = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT     _debug_messenger          = VK_NULL_HANDLE;
    VkSurfaceKHR                 _surface                  = VK_NULL_HANDLE;
    VkPhysicalDevice             _physical_device          = VK_NULL_HANDLE;
    VkDevice                     _device                   = VK_NULL_HANDLE;
    VkQueue                      _graphics_queue           = VK_NULL_HANDLE;
    VkQueue                      _present_queue            = VK_NULL_HANDLE;

    MemoryAllocator              _mem_alloc;

    //
    // use dynamic rendering
    //
    VkSwapchainKHR               _swapchain                = VK_NULL_HANDLE;
    std::vector<Image>           _swapchain_images;

    // INFO: because some device unsupport dynamic msaa feature in pipeline
    //       so we just fixed use 4bit msaa
    static constexpr VkSampleCountFlagBits _msaa_sample_count = VK_SAMPLE_COUNT_4_BIT;
    Image                        _msaa_image;
    // Image                        _msaa_depth_image;

    VkPipeline                   _2D_pipeline              = VK_NULL_HANDLE;
    VkPipelineLayout             _2D_pipeline_layout       = VK_NULL_HANDLE;

    CommandPool                  _command_pool;

    //
    // frame resources
    //
    struct FrameResource
    {
      // HACK: will I want to use command not command_buffer, but adjust them it's so terrible... after day
      Command         cmd;
      VkFence         fence               = VK_NULL_HANDLE;
      VkSemaphore     image_available_sem = VK_NULL_HANDLE; 
      VkSemaphore     render_finished_sem = VK_NULL_HANDLE; 
      //Image           depth_image;
    };

    std::vector<FrameResource>   _frames;
    uint32_t                     _current_frame            = 0;
    auto get_current_frame() -> FrameResource& { return _frames[_current_frame]; }
    
    DestructorStack              _destructors;

    // HACK: currently not use these
    VkDescriptorPool             _descriptor_pool          = VK_NULL_HANDLE;
    VkDescriptorSetLayout        _descriptor_set_layout    = VK_NULL_HANDLE;
    VkDescriptorSet              _descriptor_set           = VK_NULL_HANDLE;

    // INFO:
    // allocate a big buffer.
    // should I recreate a bigger buffer when current buffer unenough?
    Buffer                       _buffer;
  };

} }
