//
// graphics engine
//
// use Vulkan to implement
//

#pragma once

#include "Window.hpp"
#include "FrameResource.hpp"
#include "DestructorStack.hpp"
#include "Image.hpp"

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
    void draw_background(VkCommandBuffer cmd);

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
    void create_descriptor_set_layout();
    void create_compute_pipeline();
    void create_command_pool();
    void create_descriptor_pool();
    void create_descriptor_sets();
    void create_sync_objects();
    void create_frame_resources();

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
    static void copy_image(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D src_extent, VkExtent2D dst_extent);

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
    VmaAllocator                 _vma_allocator            = VK_NULL_HANDLE;
    VkQueue                      _graphics_queue           = VK_NULL_HANDLE;
    VkQueue                      _present_queue            = VK_NULL_HANDLE;

    // use dynamic rendering
    VkSwapchainKHR               _swapchain                = VK_NULL_HANDLE;
    std::vector<VkImage>         _swapchain_images;
    VkExtent2D                   _swapchain_image_extent   = {};
    Image                        _image                    = {};

    std::vector<VkPipeline>      _compute_pipeline;
    std::vector<VkPipelineLayout>_compute_pipeline_layout;

    VkCommandPool                _command_pool             = VK_NULL_HANDLE;

    //
    // frame resources
    //
    std::vector<FrameResource>   _frames;
    uint32_t                     _current_frame            = 0;
    auto get_current_frame() -> FrameResource& { return _frames[_current_frame]; }
    
    DestructorStack              _destructors;

    VkDescriptorPool             _descriptor_pool          = VK_NULL_HANDLE;
    VkDescriptorSetLayout        _descriptor_set_layout    = VK_NULL_HANDLE;
    VkDescriptorSet              _descriptor_set           = VK_NULL_HANDLE;
  };

} }
