//
// Memory Allocator
//
// use vma implement buffer and image allocate
//

#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <span>
#include <unordered_map>
#include <string_view>
#include <string>

namespace tk { namespace graphics_engine {

  class MemoryAllocator;

  class Buffer
  { 
  public:
    Buffer() = default;
    Buffer(MemoryAllocator* allocator, uint32_t size, VkBufferUsageFlags usages, VmaAllocationCreateFlags flags = 0);

    auto descriptor_buffer_usages() const noexcept { return _descriptor_buffer_usages; }

    void destroy();

    auto handle()     const noexcept { return _handle;     }
    auto allocation() const noexcept { return _allocation; }
    auto address()    const noexcept { return _address;    }
    auto data()       const noexcept { return _data;       }
    auto allocator()  const noexcept { return _allocator;  }
    auto capacity()   const noexcept { return _capacity;   }
    auto size()       const noexcept { return _size;       }

    // usefor descriptor buffer update, directly add size and tag
    
    auto add_size(uint32_t size) noexcept { _size += size; }
    void add_tag(std::string_view tag);

    auto offset(std::string const& tag) const { return _offsets.at(tag); }

    auto clear() noexcept -> Buffer&
    {
      _size = {};
      _offsets.clear();
      return *this;
    }

    auto append(void* data, uint32_t size) -> Buffer&;
    auto append(std::string_view tag, void* data, uint32_t size) -> Buffer&
    {
      add_tag(tag);
      return append(data, size);
    }

    template <typename T>
    auto append(T const& value) -> Buffer&
    {
      append(&value, sizeof(T));
      return *this;
    }
    template <typename T>
    auto append(std::string_view tag, T const& value) -> Buffer&
    {
      add_tag(tag);
      return append(value);
    }

    template <typename T>
    auto append(std::span<T> values) -> Buffer&
    {
      append(values.data(), values.size() * sizeof(T));
      return *this;
    }
    template <typename T>
    auto append(std::string_view tag, std::span<T> values) -> Buffer&
    {
      add_tag(tag);
      return append(values);
    }

  private:
    VmaAllocator    _allocator{};
    VkBuffer        _handle{};
    VmaAllocation   _allocation{};
    VkDeviceAddress _address{};
    void*           _data{};
    uint32_t        _capacity{};
    uint32_t        _size{};
    std::unordered_map<std::string, uint32_t> _offsets;
    VkBufferUsageFlags _descriptor_buffer_usages{};
  };

  class Image
  {
  public:
    Image() = default;
    Image(MemoryAllocator* allocator, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage);
    Image(VkImage handle, VkImageView view, VkExtent2D const& extent, VkFormat format)
      : _handle(handle), _view(view), _extent({ extent.width, extent.height, 1 }), _format(format) {}

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

/**
 * copy image to image, permit different format and extent
 * @param cmd command
 * @param src image
 * @param dst image
 */
void blit(Command const& cmd, Image  const& src, Image const& dst);

/**
 * copy image to image, only same format and extent
 * @param cmd command
 * @param src image
 * @param dst image
 */
void copy(Command const& cmd, Image  const& src, Image const& dst);

/**
 * copy buffer to image
 * @param cmd command
 * @param src buffer
 * @param dst image
 */
void copy(Command const& cmd, Buffer const& src, Image& dst);

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