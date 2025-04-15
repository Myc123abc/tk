//
// frame resource
//
// render one frame needs resources
//

#pragma once

#include "DestructorStack.hpp"
#include "CommandPool.hpp"

#include <vulkan/vulkan.h>

namespace tk { namespace graphics_engine {

  struct FrameResource
  {
    // HACK: will I want to use command not command_buffer, but adjust them it's so terrible... after day
    Command         command_buffer;
    VkFence         fence               = VK_NULL_HANDLE;
    VkSemaphore     image_available_sem = VK_NULL_HANDLE; 
    VkSemaphore     render_finished_sem = VK_NULL_HANDLE; 
  };

} }
