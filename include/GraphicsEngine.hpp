//
// graphics engine
//
// use Vulkan to implement
//

#pragma once

#include "Window.hpp"

#include <vk_mem_alloc.h>

#include <vector>

namespace tk
{
  class GraphicsEngine final
  {
  public:
    GraphicsEngine(Window const& window);
    ~GraphicsEngine();

    GraphicsEngine(GraphicsEngine const&)            = delete;
    GraphicsEngine(GraphicsEngine&&)                 = delete;
    GraphicsEngine& operator=(GraphicsEngine const&) = delete;
    GraphicsEngine& operator=(GraphicsEngine&&)      = delete;

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
    void create_descriptor_set_layout();
    void create_pipeline();

  private:
    // HACK: expand to multi-windows manage, use WindowManager in future.
    Window const&            _window;
    VkInstance               _instance;
    VkDebugUtilsMessengerEXT _debug_messenger;
    VkSurfaceKHR             _surface;
    VkPhysicalDevice         _physical_device;
    VkDevice                 _device;
    VkQueue                  _graphics_queue;
    VkQueue                  _present_queue;
    VmaAllocator             _vma_allocator;
    VkSwapchainKHR           _swapchain;
    std::vector<VkImage>     _swapchain_images;
    VkFormat                 _swapchain_image_format;
    VkExtent2D               _swapchain_image_extent;
    std::vector<VkImageView> _swapchain_image_views;
    VkRenderPass             _render_pass;
    VkDescriptorSetLayout    _descriptor_set_layout;
    VkPipeline               _pipeline;
    VkPipelineLayout         _pipeline_layout;
  };
}
