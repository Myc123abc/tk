#include "GraphicsEngine.hpp"
#include "ErrorHandling.hpp"
#include "Buffer.hpp"

namespace tk { namespace graphics_engine {

////////////////////////////////////////////////////////////////////////////////
//                               Buffer
////////////////////////////////////////////////////////////////////////////////

auto GraphicsEngine::begin_single_time_commands() -> VkCommandBuffer
{
  VkCommandBuffer buffer;
  VkCommandBufferAllocateInfo command_buffer_info
  {
    .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool        = _command_pool,
    .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1,
  };
  throw_if(vkAllocateCommandBuffers(_device, &command_buffer_info, &buffer) != VK_SUCCESS,
           "failed to create command buffers");

  VkCommandBufferBeginInfo beg_info
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  vkBeginCommandBuffer(buffer, &beg_info);

  return buffer;
}

void GraphicsEngine::end_single_time_commands(VkCommandBuffer command_buffer)
{
  vkEndCommandBuffer(command_buffer);

  VkSubmitInfo info
  {
    .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers    = &command_buffer,
  };

  throw_if(vkQueueSubmit(_graphics_queue, 1, &info, VK_NULL_HANDLE) != VK_SUCCESS,
           "failed to submit by queue");
  throw_if(vkQueueWaitIdle(_graphics_queue) != VK_SUCCESS,
           "failed to wait queue");

  vkFreeCommandBuffers(_device, _command_pool, 1, &command_buffer);
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
    throw_if(vmaCreateBuffer(_vma_allocator, &buffer_create_info, &alloc_create_info, &buffer, &allocation, nullptr) != VK_SUCCESS,
             "failed to create buffer");
    return;
  }

  VkBuffer          stage_buffer;
  VmaAllocation     alloc;
  throw_if(vmaCreateBuffer(_vma_allocator, &buffer_create_info, &alloc_create_info, &stage_buffer, &alloc, nullptr) != VK_SUCCESS,
           "failed to create buffer");

  // copy data to stage buffer
  throw_if(data == nullptr, "data pointer is nullptr");
  throw_if(vmaCopyMemoryToAllocation(_vma_allocator, data, alloc, 0, size) != VK_SUCCESS,
           "failed to copy data to stage buffer");

  // create vertex buffer
  buffer_create_info.usage = usage |
                             VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  alloc_create_info.flags  = 0;
  throw_if(vmaCreateBuffer(_vma_allocator, &buffer_create_info, &alloc_create_info, &buffer, &allocation, nullptr) != VK_SUCCESS,
           "failed to create buffer");
  // copy stage buffer data to vertex buffer
  copy_buffer(stage_buffer, buffer, size);

  vmaDestroyBuffer(_vma_allocator, stage_buffer, alloc);
  return;
}

Buffer GraphicsEngine::create_buffer(uint32_t size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flag)
{
  Buffer buffer;

  VkBufferCreateInfo buf_info
  {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size  = size,
    .usage = usage,
  };
  VmaAllocationCreateInfo alloc_info
  {
    .flags = flag,
    .usage = VMA_MEMORY_USAGE_AUTO,
  };
  throw_if(vmaCreateBuffer(_vma_allocator, &buf_info, &alloc_info, &buffer.buffer, &buffer.allocation, nullptr) != VK_SUCCESS,
           "failed to create buffer");

  return buffer;
}

auto GraphicsEngine::create_mesh_buffer(std::span<Vertex> vertices, std::span<uint32_t> indices) -> MeshBuffer
{
  // create mesh buffer 
  uint32_t vertices_size = vertices.size() * sizeof(Vertex);
  uint32_t indices_size  = indices.size() * sizeof(uint32_t);

  MeshBuffer mesh_buffer;
  mesh_buffer.vertices = create_buffer(vertices_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT         |
                                                      VK_BUFFER_USAGE_TRANSFER_DST_BIT           | 
                                                      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

  VkBufferDeviceAddressInfo info
  {
    .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
    .buffer = mesh_buffer.vertices.buffer,
  };
  mesh_buffer.address = vkGetBufferDeviceAddress(_device, &info);

  mesh_buffer.indices = create_buffer(indices_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | 
                                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT);

  // create stage buffer
  auto stage = create_buffer(vertices_size + indices_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
  throw_if(vmaCopyMemoryToAllocation(_vma_allocator, vertices.data(), stage.allocation, 0, vertices_size) != VK_SUCCESS,
           "failed to copy vertices data to stage buffer");
  throw_if(vmaCopyMemoryToAllocation(_vma_allocator, indices.data(), stage.allocation, vertices_size, indices_size) != VK_SUCCESS,
           "failed to copy indices data to stage buffer");

  // transform data to mesh buffer
  auto cmd = begin_single_time_commands();

  VkBufferCopy copy
  {
    .size = vertices_size,
  };
  vkCmdCopyBuffer(cmd, stage.buffer, mesh_buffer.vertices.buffer, 1, &copy);
  copy.size = indices_size;
  copy.srcOffset = vertices_size;
  vkCmdCopyBuffer(cmd, stage.buffer, mesh_buffer.indices.buffer, 1, &copy);

  end_single_time_commands(cmd);

  stage.destroy(_vma_allocator);

  return mesh_buffer;
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
