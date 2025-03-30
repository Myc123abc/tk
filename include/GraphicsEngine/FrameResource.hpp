//
// frame resource
// render one frame needs resources
//

#pragma once

#include <vulkan/vulkan.h>

namespace tk { namespace graphics_engine {

  struct FrameResource
  {
  // public:
  //   FrameResource();
  //   ~FrameResource();
  //
  //   FrameResource(FrameResource const&)            = delete;
  //   FrameResource(FrameResource&&)                 = delete;
  //   FrameResource& operator=(FrameResource const&) = delete;
  //   FrameResource& operator=(FrameResource&&)      = delete;
  //
  // private:
    VkCommandBuffer command_buffer      = VK_NULL_HANDLE;
    VkFence         fence               = VK_NULL_HANDLE;
    VkSemaphore     image_available_sem = VK_NULL_HANDLE; 
    VkSemaphore     render_finished_sem = VK_NULL_HANDLE; 
  };

} }
