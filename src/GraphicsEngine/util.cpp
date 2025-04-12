#include "GraphicsEngine.hpp"
#include "ErrorHandling.hpp"

namespace tk { namespace graphics_engine {

////////////////////////////////////////////////////////////////////////////////
//                               Buffer
////////////////////////////////////////////////////////////////////////////////

auto GraphicsEngine::begin_single_time_commands() -> VkCommandBuffer
{
  return _command_pool.create_command().begin();
}

void GraphicsEngine::end_single_time_commands(VkCommandBuffer command_buffer)
{
  Command(command_buffer).end().submit_wait_free(_command_pool, _graphics_queue);
}

void GraphicsEngine::copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) 
{
  // create temporary command buffer to transfer data from stage buffer to device local buffer
  auto buf = begin_single_time_commands();

  // record transfer data command
  VkBufferCopy copy_region =
  {
    .size = size,
  };
  vkCmdCopyBuffer(buf, src, dst, 1, &copy_region);

  // end record command
  end_single_time_commands(buf);
}

void GraphicsEngine::create_buffer(VkBuffer& buffer, VmaAllocation& allocation, uint32_t size, VkBufferUsageFlags usage, void const* data)
{
  // create stage buffer
  VkBufferCreateInfo buffer_create_info
  {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size  = size,
    .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
  };

  VmaAllocationCreateInfo alloc_create_info
  {
    .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    .usage = VMA_MEMORY_USAGE_AUTO,
  };

  if (data == nullptr)
  {
    buffer_create_info.usage = usage;
    alloc_create_info.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
    throw_if(vmaCreateBuffer(_mem_alloc.get(), &buffer_create_info, &alloc_create_info, &buffer, &allocation, nullptr) != VK_SUCCESS,
             "failed to create buffer");
    return;
  }

  VkBuffer          stage_buffer;
  VmaAllocation     alloc;
  throw_if(vmaCreateBuffer(_mem_alloc.get(), &buffer_create_info, &alloc_create_info, &stage_buffer, &alloc, nullptr) != VK_SUCCESS,
           "failed to create buffer");

  // copy data to stage buffer
  throw_if(data == nullptr, "data pointer is nullptr");
  throw_if(vmaCopyMemoryToAllocation(_mem_alloc.get(), data, alloc, 0, size) != VK_SUCCESS,
           "failed to copy data to stage buffer");

  // create vertex buffer
  buffer_create_info.usage = usage |
                             VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  alloc_create_info.flags  = 0;
  throw_if(vmaCreateBuffer(_mem_alloc.get(), &buffer_create_info, &alloc_create_info, &buffer, &allocation, nullptr) != VK_SUCCESS,
           "failed to create buffer");
  // copy stage buffer data to vertex buffer
  copy_buffer(stage_buffer, buffer, size);

  vmaDestroyBuffer(_mem_alloc.get(), stage_buffer, alloc);
  return;
}


////////////////////////////////////////////////////////////////////////////////
//                               Image 
////////////////////////////////////////////////////////////////////////////////

void GraphicsEngine::transition_image_layout(VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout)
{
  auto aspect_mask = new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ?
                     VK_IMAGE_ASPECT_DEPTH_BIT :
                     VK_IMAGE_ASPECT_COLOR_BIT;

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
    .oldLayout        = old_layout,
    .newLayout        = new_layout,
    .image            = image,
    .subresourceRange = get_image_subresource_range(aspect_mask),
  };

  VkDependencyInfo dep_info
  {
    .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    .imageMemoryBarrierCount = 1,
    .pImageMemoryBarriers    = &barrier,
  };

  vkCmdPipelineBarrier2(cmd, &dep_info);
}

auto GraphicsEngine::get_image_subresource_range(VkImageAspectFlags aspect) -> VkImageSubresourceRange
{
  return
  {
    .aspectMask = aspect, 
    .levelCount = VK_REMAINING_MIP_LEVELS,
    .layerCount = VK_REMAINING_ARRAY_LAYERS,
  };
}

// HACK: VkCmdCopyImage can be faster but most restriction such as src and dst are same format and extent. 
void GraphicsEngine::copy_image(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D src_extent, VkExtent2D dst_extent)
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
  blit.srcOffsets[1].x = src_extent.width;
  blit.srcOffsets[1].y = src_extent.height;
  blit.srcOffsets[1].z = 1;
  blit.dstOffsets[1].x = dst_extent.width;
  blit.dstOffsets[1].y = dst_extent.height;
  blit.dstOffsets[1].z = 1;

  VkBlitImageInfo2 info
  {
    .sType          = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
    .srcImage       = src,
    .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    .dstImage       = dst,
    .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    .regionCount    = 1,
    .pRegions       = &blit,
    .filter         = VK_FILTER_LINEAR,
  };

  vkCmdBlitImage2(cmd, &info);
}

} }
