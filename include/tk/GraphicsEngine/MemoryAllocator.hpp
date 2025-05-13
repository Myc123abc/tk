//
// Memory Allocator
//
// use vma implement buffer and image allocate
//

#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace tk { namespace graphics_engine {

  class MemoryAllocator;

  class Buffer
  { 
  public:
    Buffer() = default;
    Buffer(MemoryAllocator* allocator, uint32_t size, VkBufferUsageFlags usages, VmaAllocationCreateFlags flags = 0);

    void destroy();

    auto handle()     const noexcept { return _handle;     }
    auto allocation() const noexcept { return _allocation; }
    auto address()    const noexcept { return _address;    }
    auto data()       const noexcept { return _data;       }

    // TODO: internal record every memory block type, offset and auto increase capacity

  private:
    VmaAllocator    _allocator  = {};
    VkBuffer        _handle     = {};
    VmaAllocation   _allocation = {};
    VkDeviceAddress _address    = {};
    void*           _data       = {};
    uint32_t        _capacity   = {};
    uint32_t        _size       = {};
  };

  struct Image
  {
    VkImage       handle     = {};
    VkImageView   view       = {};
    VmaAllocation allocation = {};
    VkExtent3D    extent     = {};
    VkFormat      format     = {};
  };

  class MemoryAllocator
  {
    friend class Buffer;
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

    auto create_buffer(uint32_t size, VkBufferUsageFlags usages, VmaAllocationCreateFlags flags = 0) {  return Buffer(this, size, usages, flags); }

    auto create_image(VkFormat format, VkExtent3D extent, VkImageUsageFlags usage, VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT) -> Image;
    void destroy_image(Image& image);

    // auto create_mesh_buffer(class Command& command, std::vector<class Mesh>& meshs, DestructorStack& destructor, std::vector<MeshInfo>& mesh_infos) -> MeshBuffer;
    // void destroy_mesh_buffer(MeshBuffer& buffer);

  private:
    VkDevice     _device    = VK_NULL_HANDLE;
    VmaAllocator _allocator = VK_NULL_HANDLE;
  };

}}