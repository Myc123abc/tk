//
// image
// 
// image struct
//

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace tk { namespace graphics_engine {

  struct Image
  {
    VkImage       image      = VK_NULL_HANDLE;
    VkImageView   view       = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkExtent3D    extent     = {};
    VkFormat      format     = VK_FORMAT_UNDEFINED;
  };

} }
