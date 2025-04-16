//
// graphics engine
//
// use Vulkan to implement
//
// TODO:
// 1. use offscreen rendering, but when window size bigger than image size, it will be stretch,
//    maybe need to recreate image in this case.
//

#pragma once

#include "Window.hpp"
#include "FrameResource.hpp"
#include "DestructorStack.hpp"
#include "Image.hpp"
#include "MemoryAllocator.hpp"
#include "CommandPool.hpp"
#include "Painter.hpp"

#include <vk_mem_alloc.h>
#include <SDL3/SDL_events.h>

#include <vector>

namespace tk { namespace graphics_engine {

  // HACK: discard because of painter
  struct ShapeInfo
  {
    uint32_t     indices_offset;
    uint32_t     indices_count;
  };

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
    // maybe update and draw should be user's codes
    // also with keyboard_process
    void update();
    void draw();
    void resize_swapchain();

  private:
    void draw(VkCommandBuffer cmd);
    void draw_background(Command cmd);
    void painter_to_draw();

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
    void init_painter();
    // HACK: subtle naming...
    void use_single_time_command_init_something();

    //
    // util 
    //
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
    VkQueue                      _graphics_queue           = VK_NULL_HANDLE;
    VkQueue                      _present_queue            = VK_NULL_HANDLE;

    MemoryAllocator              _mem_alloc;

    // FIX: discard
    //      Painter and background id should in UI class
    Painter                      _painter; 
    uint32_t                     _background_id = -1;

    // use dynamic rendering
    VkSwapchainKHR               _swapchain                = VK_NULL_HANDLE;
    std::vector<VkImage>         _swapchain_images;
    VkExtent2D                   _swapchain_image_extent   = {};
    Image                        _image                    = {};
    Image                        _depth_image              = {};
    VkExtent2D                   _draw_extent              = {};

    VkPipeline                   _2D_pipeline              = VK_NULL_HANDLE;
    VkPipelineLayout             _2D_pipeline_layout       = VK_NULL_HANDLE;

    CommandPool                  _command_pool;

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
