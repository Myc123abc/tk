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

    bool is_valid() const noexcept { return _handle; }

  private:
    VmaAllocator    _allocator  = {};
    VkBuffer        _handle     = {};
    VmaAllocation   _allocation = {};
    VkDeviceAddress _address    = {};
    void*           _data       = {};
  };

  class Image
  {
  public:
    Image() = default;
    Image(MemoryAllocator* allocator, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage);
    Image(VkImage handle, VkImageView view, VkExtent2D const& extent, VkFormat format)
      : _handle(handle), _view(view), _extent({ extent.width, extent.height }), _format(format) {}

    void destroy();

    auto handle()     const noexcept { return _handle;     }
    auto view()       const noexcept { return _view;       }
    auto allocation() const noexcept { return _allocation; }
    auto extent3D()   const noexcept { return _extent;     }
    auto extent2D()   const noexcept { return VkExtent2D{ _extent.width, _extent.height }; }
    auto format()     const noexcept { return _format;     }

    void set_layout(class Command const& cmd, VkImageLayout layout);
    void clear(class Command const& cmd, VkClearColorValue value = {});

  private:
    VmaAllocator  _allocator  = {};
    VkDevice      _device     = {};
    VkImage       _handle     = {};
    VkImageView   _view       = {};
    VmaAllocation _allocation = {};
    VkExtent3D    _extent     = {};
    VkFormat      _format     = {};
    VkImageLayout _layout     = VK_IMAGE_LAYOUT_UNDEFINED;
  };

void blit_image(Command const& cmd, Image const& src, Image const& dst);
void copy_image(Command const& cmd, Image const& src, Image const& dst);

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

    auto get()    const noexcept { return _allocator; }
    auto device() const noexcept { return _device;    }

    auto create_buffer(uint32_t size, VkBufferUsageFlags usages, VmaAllocationCreateFlags flags = 0) { return Buffer(this, size, usages, flags);  }
    auto create_image(VkFormat format, VkExtent3D extent, VkImageUsageFlags usage)                   { return Image(this, format, extent, usage); }

  private:
    VkDevice     _device    = VK_NULL_HANDLE;
    VmaAllocator _allocator = VK_NULL_HANDLE;
  };

}}