//
// graphics engine
//
// use Vulkan to implement
//

#pragma once

#include "Window.hpp"
#include "FrameResource.hpp"

#include <vk_mem_alloc.h>

#include <vector>

namespace tk { namespace graphics_engine {

  // TODO: graphics engine only initialize.
  // you need add vertices, indices, uniform, and shaders to run it.
  class GraphicsEngine
  {
  public:
    GraphicsEngine(Window const& window);
    ~GraphicsEngine();

    GraphicsEngine(GraphicsEngine const&)            = delete;
    GraphicsEngine(GraphicsEngine&&)                 = delete;
    GraphicsEngine& operator=(GraphicsEngine const&) = delete;
    GraphicsEngine& operator=(GraphicsEngine&&)      = delete;

    //
    // run
    //
    // HACK: this not should in here, it should in use engine's place
    void run();
  private:
    void update();
    void draw();
    void record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index);

  private:
    //
    // initialize resources
    //
    void create_instance();
    void create_debug_messenger();
    void create_surface();
    void select_physical_device();
    void create_device_and_get_queues();
    void create_vma_allocator();
    void create_swapchain_and_get_swapchain_images_info();
    void create_swapchain_image_views();
    void create_render_pass();
    void create_framebuffers();
    void create_descriptor_set_layout();
    void create_pipeline();
    void create_command_pool();
    void create_buffers();
    void create_descriptor_pool();
    void create_descriptor_sets();
    void create_sync_objects();

    void create_frame_resources();
    void destroy_frame_resources();

    //
    // util 
    //
    // HACK: repeat single command, performance bad
    auto begin_single_time_commands() -> VkCommandBuffer;
    void end_single_time_commands(VkCommandBuffer command_buffer);
    void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
    // HACK: suballoc and single buffer
    void create_buffer(VkBuffer& buffer, VmaAllocation& allocation, 
                       uint32_t size, VkBufferUsageFlags usage,
                       void const* data = nullptr);

    static void transition_image_layout(VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout);
    static auto get_image_subresource_range(VkImageAspectFlags aspect) -> VkImageSubresourceRange;

  private:
    //
    // common resources
    //
    // HACK: expand to multi-windows manage, use WindowManager in future.
    Window const&                _window;
    VkInstance                   _instance                 = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT     _debug_messenger          = VK_NULL_HANDLE;
    VkSurfaceKHR                 _surface                  = VK_NULL_HANDLE;
    VkPhysicalDevice             _physical_device          = VK_NULL_HANDLE;
    VkDevice                     _device                   = VK_NULL_HANDLE;
    VkQueue                      _graphics_queue           = VK_NULL_HANDLE;
    VkQueue                      _present_queue            = VK_NULL_HANDLE;
    VmaAllocator                 _vma_allocator            = VK_NULL_HANDLE;
    VkSwapchainKHR               _swapchain                = VK_NULL_HANDLE;
    std::vector<VkImage>         _swapchain_images;
    VkFormat                     _swapchain_image_format   = VK_FORMAT_UNDEFINED;
    VkExtent2D                   _swapchain_image_extent   = {};
    std::vector<VkImageView>     _swapchain_image_views;  
    VkRenderPass                 _render_pass              = VK_NULL_HANDLE;
    // framebuffer is used on swapchain image,
    // which will be presented on screen when commanded to queue.
    // it's not the concept of frame resource,
    // which is use for CPU to handle every frame.
    // frambuffer is handled by GPU btw.
    std::vector<VkFramebuffer>   _framebuffers;
    VkDescriptorSetLayout        _descriptor_set_layout    = VK_NULL_HANDLE;
    VkPipeline                   _pipeline                 = VK_NULL_HANDLE;
    VkPipelineLayout             _pipeline_layout          = VK_NULL_HANDLE;
    VkCommandPool                _command_pool             = VK_NULL_HANDLE;

    //
    // frame resources
    //
    std::vector<FrameResource>   _frames;
    uint32_t                     _current_frame            = 0;
    auto get_current_frame() -> FrameResource& { return _frames[_current_frame]; }
    
    // HACK: use suballoc memory and single buffer
    VkBuffer                     _vertex_buffer            = VK_NULL_HANDLE;
    VmaAllocation                _vertex_buffer_allocation = VK_NULL_HANDLE;
    VkBuffer                     _index_buffer             = VK_NULL_HANDLE;
    VmaAllocation                _index_buffer_allocation  = VK_NULL_HANDLE;
    std::vector<VkBuffer>        _uniform_buffers;
    std::vector<VmaAllocation>   _uniform_buffer_allocations;

    // HACK: make frame resource for anything in a frame
    VkDescriptorPool             _descriptor_pool          = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> _descriptor_sets;
  };

} }
