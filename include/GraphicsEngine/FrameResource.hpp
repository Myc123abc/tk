//
// frame
// render one frame needs resources
//

#pragma once

#include <vulkan/vulkan.h>

namespace tk { namespace graphics_engine {

  class Frame
  {
  public:
    Frame();
    ~Frame();

    Frame(Frame const&)            = delete;
    Frame(Frame&&)                 = delete;
    Frame& operator=(Frame const&) = delete;
    Frame& operator=(Frame&&)      = delete;

  private:
    VkCommandPool   _command_pool  { VK_NULL_HANDLE };
    VkCommandBuffer _command_buffer{ VK_NULL_HANDLE };
  };

} }
