#include "tk/GraphicsEngine/MemoryAllocator.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/GraphicsEngine/config.hpp"

#include <cassert>

namespace tk { namespace graphics_engine {

////////////////////////////////////////////////////////////////////////////////
//                                  Buffer
////////////////////////////////////////////////////////////////////////////////

void Buffer::destroy() const
{
  assert(_allocator && _handle && _allocation);
  vmaDestroyBuffer(_allocator->get(), _handle, _allocation);
}

Buffer::Buffer(MemoryAllocator* allocator, uint32_t size, VkBufferUsageFlags usages, VmaAllocationCreateFlags flags) 
{
  _allocator = allocator;
  _capacity  = size;
  _usages    = usages;
  _flags     = flags;
  
  // for dynamic expand, use mapped
  flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
  
  VkBufferCreateInfo buffer_create_info
  {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size  = size,
    .usage = usages,
  };
  VmaAllocationCreateInfo allocation_create_info
  {
    .flags = flags,
    .usage = VMA_MEMORY_USAGE_AUTO,
  };
  VmaAllocationInfo allocation_info;
  throw_if(vmaCreateBuffer(_allocator->get(), &buffer_create_info, &allocation_create_info, &_handle, &_allocation, &allocation_info) != VK_SUCCESS,
           "failed to create buffer");
  assert(allocation_info.pMappedData);
  _data = allocation_info.pMappedData;

  if (usages & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
  {
    // get address
    VkBufferDeviceAddressInfo info
    {
      .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      .buffer = _handle,
    };
    _address = vkGetBufferDeviceAddress(allocator->_device, &info);
  }
}

void Buffer::add_tag(std::string const& tag)
{
  throw_if(_offsets.emplace(tag, _size).second == false, "already have tag {}", tag);
}

auto Buffer::append(void const* data, uint32_t size) -> Buffer&
{
  auto total_size = _size + size;
  if (total_size <= _capacity)
  {
    throw_if(vmaCopyMemoryToAllocation(_allocator->get(), data, _allocation, _size, size) != VK_SUCCESS,
             "failed to copy data to upload buffer");
    _size = total_size;
  }
  // unenough, expand capacity
  else
  {
    auto tmp_buf = Buffer(_allocator, std::max(total_size, static_cast<uint32_t>(_capacity * config()->buffer_expand_ratio)), _usages, _flags);
    tmp_buf.append(_data, _size)
           .append(data, size);
    // destroy old buffer
    destroy();
    set_realloc_info(tmp_buf);
  }
  return *this;
}

auto Buffer::set_realloc_info(Buffer& buf) -> Buffer*
{
  _handle     = buf._handle;
  _allocation = buf._allocation;
  _address    = buf._address;
  _data       = buf._data;
  _capacity   = buf._capacity;
  _size       = buf._size;
  return this;
}

////////////////////////////////////////////////////////////////////////////////
//                                  Image
////////////////////////////////////////////////////////////////////////////////

Image::Image(MemoryAllocator* allocator, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage)
{
  _allocator = allocator->get();
  _device    = allocator->device();
  _format    = format;
  _extent    = extent;

  VkImageCreateInfo image_info
  {
    .sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType   = VK_IMAGE_TYPE_2D,
    .format      = _format,
    .extent      = _extent,
    .mipLevels   = 1,
    .arrayLayers = 1,
    .samples     = VK_SAMPLE_COUNT_1_BIT,
    .tiling      = VK_IMAGE_TILING_OPTIMAL,
    .usage       = usage,
  };

  VmaAllocationCreateInfo alloc_info
  {
    .flags         = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    .usage         = VMA_MEMORY_USAGE_AUTO,
  };
  throw_if(vmaCreateImage(_allocator, &image_info, &alloc_info, &_handle, &_allocation, nullptr) != VK_SUCCESS,
           "failed to create image");

  VkImageViewCreateInfo image_view_info
  {
    .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image    = _handle,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format   = _format,
    .subresourceRange =
    {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .levelCount = 1,
      .layerCount = 1,
    },
  };
  throw_if(vkCreateImageView(allocator->device(), &image_view_info, nullptr, &_view) != VK_SUCCESS,
           "failed to create image view");
}

auto Image::set_layout(class Command const& cmd, VkImageLayout layout) -> Image&
{
  if (_layout == layout) return *this;
  VkImageMemoryBarrier2 barrier
  {
    .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
    // HACK: use all commands bit will stall the GPU pipeline a bit, is inefficient.
    // should make stageMask more accurate.
    // reference: https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples
    .srcStageMask     = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
    .srcAccessMask    = VK_ACCESS_2_MEMORY_WRITE_BIT,
    .dstStageMask     = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
    .dstAccessMask    = VK_ACCESS_2_MEMORY_READ_BIT  |
                        VK_ACCESS_2_MEMORY_WRITE_BIT,
    .oldLayout        = _layout,
    .newLayout        = layout,
    .image            = _handle,
    .subresourceRange =  
    { 
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, 
      .levelCount = VK_REMAINING_MIP_LEVELS,
      .layerCount = VK_REMAINING_ARRAY_LAYERS,
    }
  };
  VkDependencyInfo dep_info
  {
    .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    .imageMemoryBarrierCount = 1,
    .pImageMemoryBarriers    = &barrier,
  };
  vkCmdPipelineBarrier2(cmd, &dep_info);
  _layout = layout;
  return *this;
}

void Image::destroy()
{
  assert(_handle && _allocation);

  vkDestroyImageView(_device, _view, nullptr);
  vmaDestroyImage(_allocator, _handle, _allocation);

  _allocator  = {};
  _device     = {};
  _handle     = {};
  _view       = {};
  _allocation = {};
  _extent     = {};
  _format     = {};
  _layout     = VK_IMAGE_LAYOUT_UNDEFINED;
}

auto Image::clear(class Command const& cmd, VkClearColorValue value) -> Image&
{
  set_layout(cmd, VK_IMAGE_LAYOUT_GENERAL);
  VkImageSubresourceRange range
  {
    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .levelCount = VK_REMAINING_MIP_LEVELS,
    .layerCount = VK_REMAINING_ARRAY_LAYERS,
  };
  vkCmdClearColorImage(cmd, _handle, VK_IMAGE_LAYOUT_GENERAL, &value, 1, &range);
  return *this;
}

////////////////////////////////////////////////////////////////////////////////
//                                 util
////////////////////////////////////////////////////////////////////////////////

void blit(Command const& cmd, Image const& src, Image const& dst)
{
  VkImageBlit2 blit
  { 
    .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
    .srcSubresource =
    {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .layerCount = 1,
    },
    .dstSubresource =
    {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .layerCount = 1,
    },
  };
  blit.srcOffsets[1].x = src.extent3D().width;
  blit.srcOffsets[1].y = src.extent3D().height;
  blit.srcOffsets[1].z = 1;
  blit.dstOffsets[1].x = dst.extent3D().width;
  blit.dstOffsets[1].y = dst.extent3D().height;
  blit.dstOffsets[1].z = 1;

  VkBlitImageInfo2 info
  {
    .sType          = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
    .srcImage       = src.handle(),
    .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    .dstImage       = dst.handle(),
    .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    .regionCount    = 1,
    .pRegions       = &blit,
    .filter         = VK_FILTER_LINEAR,
  };

  vkCmdBlitImage2(cmd, &info);
}

void copy(Command const& cmd, Image const& src, Image const& dst)
{
  VkImageCopy2 region
  {
    .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
    .srcSubresource =
    {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .layerCount = 1,
    },
    .dstSubresource =
    {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .layerCount = 1,
    },
    .extent = src.extent3D(),
  };
  VkCopyImageInfo2 info
  {
    .sType          = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
    .srcImage       = src.handle(),
    .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    .dstImage       = dst.handle(),
    .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    .regionCount    = 1,
    .pRegions       = &region,
  };
  vkCmdCopyImage2(cmd, &info);
}

void copy(Command const& cmd, Buffer const& src, Image& dst)
{
  dst.set_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  VkBufferImageCopy2 region
  {
    .sType            = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
    .bufferOffset     = 0,
    .imageSubresource =
    {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .layerCount = 1,
    },
    .imageExtent = dst.extent3D(),
  };
  VkCopyBufferToImageInfo2 info
  {
    .sType          = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
    .srcBuffer      = src.handle(),
    .dstImage       = dst.handle(),
    .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    .regionCount    = 1,
    .pRegions       = &region,
  };
  vkCmdCopyBufferToImage2(cmd, &info);
}

void copy(Command const& cmd, Buffer const& src, uint32_t buffer_offset, Image& dst, VkOffset2D image_offset, VkExtent2D extent)
{
  dst.set_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  VkBufferImageCopy2 region
  {
    .sType             = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
    .bufferOffset      = buffer_offset,
    .bufferRowLength   = extent.width,
    .bufferImageHeight = extent.height,
    .imageSubresource  =
    {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .layerCount = 1,
    },
    .imageOffset = { image_offset.x, image_offset.y, 0 },
    .imageExtent = { extent.width,   extent.height,  1 },
  };
  VkCopyBufferToImageInfo2 info
  {
    .sType          = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
    .srcBuffer      = src.handle(),
    .dstImage       = dst.handle(),
    .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    .regionCount    = 1,
    .pRegions       = &region,
  };
  vkCmdCopyBufferToImage2(cmd, &info);
}

void copy(Command const& cmd, Buffer const& src, Image& dst, std::span<CopyRegion const> regions)
{
  if (regions.empty()) return;
  dst.set_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  auto copy_regions = std::vector<VkBufferImageCopy2>(regions.size());
  for (auto i = 0; i < regions.size(); ++i)
  {
    auto& copy_region = copy_regions[i];
    auto& region      = regions[i];
    copy_region.sType             = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
    copy_region.bufferOffset      = region.buffer_offset;
    copy_region.bufferRowLength   = region.extent.width;
    copy_region.bufferImageHeight = region.extent.height;
    copy_region.imageSubresource  =
    {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .layerCount = 1,
    };
    copy_region.imageOffset = { region.image_offset.x, region.image_offset.y, 0 };
    copy_region.imageExtent = { region.extent.width,   region.extent.height,  1 };
  }

  VkCopyBufferToImageInfo2 info
  {
    .sType          = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
    .srcBuffer      = src.handle(),
    .dstImage       = dst.handle(),
    .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    .regionCount    = static_cast<uint32_t>(copy_regions.size()),
    .pRegions       = copy_regions.data(),
  };
  vkCmdCopyBufferToImage2(cmd, &info);
}

////////////////////////////////////////////////////////////////////////////////
//                              Memory Allocator
////////////////////////////////////////////////////////////////////////////////

void MemoryAllocator::init(VkPhysicalDevice physical_device, VkDevice device, VkInstance instance, uint32_t vulkan_version)
{
  static bool only_one = true;
  if (only_one) only_one = false;
  else throw_if(true, "only single memory allocator be permitted");

  _device = device;
  VmaAllocatorCreateInfo alloc_info
  {
    .flags            = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT |
                        VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
    .physicalDevice   = physical_device,
    .device           = device,
    .instance         = instance,
    .vulkanApiVersion = vulkan_version,
  };
  throw_if(vmaCreateAllocator(&alloc_info, &_allocator) != VK_SUCCESS,
           "failed to create Vulkan Memory Allocator");
}

void MemoryAllocator::destroy()
{
  if (_allocator != VK_NULL_HANDLE)
    vmaDestroyAllocator(_allocator);
  _device    = VK_NULL_HANDLE;
  _allocator = VK_NULL_HANDLE;
}

}}
