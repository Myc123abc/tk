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
#include "Buffer.hpp"
#include "gltf.hpp"

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
    void keyboard_process(SDL_KeyboardEvent const& key);

    // HACK: 32bit indices? not 16bit?
    auto create_mesh_buffer(std::span<Vertex> vertices, std::span<uint32_t> indices) -> MeshBuffer;

  private:
    void draw_background(VkCommandBuffer cmd);
    void draw_geometry(VkCommandBuffer cmd);

    uint32_t _pipeline_index = 0;

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
    void create_swapchain_and_rendering_image();
    void create_swapchain(VkSwapchainKHR old_swapchain = VK_NULL_HANDLE);
    void create_descriptor_set_layout();
    void create_compute_pipeline();
    void create_graphics_pipeline();
    void create_command_pool();
    void create_descriptor_pool();
    void create_descriptor_sets();
    void create_sync_objects();
    void create_frame_resources();

    void resize_swapchain();

    void upload_data();

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

    auto create_buffer(uint32_t size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flag = 0) -> Buffer;

    static void transition_image_layout(VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout);
    static auto get_image_subresource_range(VkImageAspectFlags aspect) -> VkImageSubresourceRange;
    static void copy_image(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D src_extent, VkExtent2D dst_extent);

    void load_gltf();

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
    Image                        _depth_image              = {};
    VkExtent2D                   _draw_extent              = {};

    std::vector<VkPipeline>      _compute_pipeline;
    std::vector<VkPipelineLayout>_compute_pipeline_layout;
    VkPipeline                   _graphics_pipeline        = VK_NULL_HANDLE;
    VkPipelineLayout             _graphics_pipeline_layout = VK_NULL_HANDLE;
    VkPipeline                   _mesh_pipeline            = VK_NULL_HANDLE;
    VkPipelineLayout             _mesh_pipeline_layout     = VK_NULL_HANDLE;
    MeshBuffer                   _mesh_buffer;

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

    // mesh
    std::vector<std::shared_ptr<MeshAsset>> _meshs;
    int x = 0, y = 0, z = 0;
  };

} }
