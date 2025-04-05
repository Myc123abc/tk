//
// buffer
//
// use vma
//

#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

namespace tk { namespace graphics_engine {

  struct Buffer
  {
    VkBuffer          buffer          = VK_NULL_HANDLE;
    VmaAllocation     allocation      = VK_NULL_HANDLE;

    void destroy(VmaAllocator allocator) { vmaDestroyBuffer(allocator, buffer, allocation); }
  };

  struct Vertex
  {
    // glm::vec3 pos;
    // glm::vec3 normal;
    // glm::vec4 color;
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 normal;
    alignas(16) glm::vec4 color;
  };

  struct MeshBuffer
  {
    Buffer          vertices;
    Buffer          indices;
    VkDeviceAddress address = {};

    void destroy(VmaAllocator allocator)
    {
      vertices.destroy(allocator);
      indices.destroy(allocator);
    }
  };

  struct GeometryPushConstant
  {
    glm::mat4       world_matrix;
    VkDeviceAddress address = {};
  };

} }
