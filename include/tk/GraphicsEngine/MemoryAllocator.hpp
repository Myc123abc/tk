//
// Memory Allocator
//
// use vma implement buffer and image allocate
//

#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

#include <unordered_map>
#include <string>
#include <ranges>

namespace tk { namespace graphics_engine {

  template <typename T>
  concept NotContainer = 
    !std::ranges::range<T> && std::is_trivially_copyable_v<T>;

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
    void add_tag(std::string const& tag);

    auto offset(std::string const& tag) const { return _offsets.at(tag); }

    auto clear() noexcept -> Buffer&
    {
      _size = {};
      _offsets.clear();
      return *this;
    }

    auto append(void const* data, uint32_t size) -> Buffer&;
    auto append(std::string const& tag, void const* data, uint32_t size) -> Buffer&
    {
      add_tag(tag);
      return append(data, size);
    }

    template <NotContainer T>
    auto append(T const& value) -> Buffer&
    {
      append(&value, sizeof(T));
      return *this;
    }
    template <NotContainer T>
    auto append(std::string const& tag, T const& value) -> Buffer&
    {
      add_tag(tag);
      return append(value);
    }

    template <std::ranges::range R>
    requires (std::is_trivially_copyable_v<std::ranges::range_value_t<R>>)
    auto append_range(R&& values) -> Buffer&
    {
      using T = std::ranges::range_value_t<R>;

      if constexpr (std::ranges::sized_range<R>)
      {
        auto count = std::ranges::size(values);
        if (count)
          append(std::ranges::data(values), count * sizeof(T));
      } 
      else
        for (T const& val : values)
          append(&val, sizeof(T));
      return *this;
    }

    template <std::ranges::range R>
    requires (std::is_trivially_copyable_v<std::ranges::range_value_t<R>>)
    auto append_range(std::string const& tag, std::span<R> values) -> Buffer&
    {
      add_tag(tag);
      return append_range(values);
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

    auto set_layout(class Command const& cmd, VkImageLayout layout)    -> Image&;
    auto clear(class Command const& cmd, VkClearColorValue value = {}) -> Image&;

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
// TODO: make pairs, multiple copy in once call
void copy(Command const& cmd, Buffer const& src, uint32_t buffer_offset, Image& dst, VkOffset2D image_offset, VkExtent2D extent);
inline void copy(Command const& cmd, Buffer const& src, uint32_t buffer_offset, Image& dst, glm::vec2 image_offset, glm::vec2 extent)
{
  copy(cmd, src, buffer_offset, dst,
    VkOffset2D{ static_cast<int32_t>(image_offset.x), static_cast<int32_t>(image_offset.y) },
    VkExtent2D{ static_cast<uint32_t>(extent.x), static_cast<uint32_t>(extent.y) });
}

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
    auto create_image(VkFormat format, uint32_t width, uint32_t height, VkImageUsageFlags usage) { return Image(this, format, { width, height, 1 }, usage); }

  private:
    VkDevice     _device    = VK_NULL_HANDLE;
    VmaAllocator _allocator = VK_NULL_HANDLE;
  };

}}