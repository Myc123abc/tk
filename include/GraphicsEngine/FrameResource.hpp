//
// frame resource
// render one frame needs resources
//

#pragma once

#include "DestructorStack.hpp"

#include <vulkan/vulkan.h>

namespace tk { namespace graphics_engine {

  struct FrameResource
  {
    VkCommandBuffer command_buffer      = VK_NULL_HANDLE;
    VkFence         fence               = VK_NULL_HANDLE;
    VkSemaphore     image_available_sem = VK_NULL_HANDLE; 
    VkSemaphore     render_finished_sem = VK_NULL_HANDLE; 

    DestructorStack destructors;
  };

} }
