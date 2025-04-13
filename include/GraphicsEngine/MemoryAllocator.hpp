//
// Memory Allocator
//
// use vma implement buffer and image allocate
//

#pragma once

#include "DestructorStack.hpp"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>

namespace tk { namespace graphics_engine {

  struct Buffer
  { 
    VkBuffer      handle     = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
  };

  struct MeshBuffer
  {
    Buffer          vertices;
    Buffer          indices;
    VkDeviceAddress address = {};
  };

  struct MeshInfo
  {
    uint32_t vertices_offset;
    uint32_t inidces_offset;
    uint32_t indices_count;
  };

  class MemoryAllocator
  {
  public:
    MemoryAllocator()  = default;
    ~MemoryAllocator() = default;

    MemoryAllocator(MemoryAllocator const&)            = delete;
    MemoryAllocator(MemoryAllocator&&)                 = delete;
    MemoryAllocator& operator=(MemoryAllocator const&) = delete;
    MemoryAllocator& operator=(MemoryAllocator&&)      = delete;

    void init(VkPhysicalDevice physical_device, VkDevice device, VkInstance instance, uint32_t vulkan_version);
    void destroy();

    // HACK: tmp, use for old code, should be discard
    auto get() { return _allocator; }

    auto create_buffer(uint32_t size, VkBufferUsageFlags usages, VmaAllocationCreateFlags flags = 0) -> Buffer;
    void destroy_buffer(Buffer& buffer);

    auto create_mesh_buffer(class Command& command, std::vector<class Mesh>& meshs, DestructorStack& destructor, std::vector<MeshInfo>& mesh_infos) -> MeshBuffer;
    void destroy_mesh_buffer(MeshBuffer& buffer);

  private:
    VkDevice     _device    = VK_NULL_HANDLE;
    VmaAllocator _allocator = VK_NULL_HANDLE;
  };

}}
