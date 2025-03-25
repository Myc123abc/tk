#include "GraphicsEngine.hpp"
#include "ErrorHandling.hpp"

namespace tk
{

VkCommandBuffer GraphicsEngine::begin_single_time_commands()
{
  VkCommandBufferAllocateInfo alloc_info
  {
    .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool        = _command_pool,
    .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1,
  };

  VkCommandBuffer command_buffer;
  throw_if(vkAllocateCommandBuffers(_device, &alloc_info, &command_buffer) != VK_SUCCESS,
           "failed to allocate command buffers");

  VkCommandBufferBeginInfo beg_info
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };

  vkBeginCommandBuffer(command_buffer, &beg_info);

  return command_buffer;
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

}
