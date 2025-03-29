#include "CommandPool.hpp"
#include "ErrorHandling.hpp"

#include <ranges>

namespace tk { namespace graphics_engine {

////////////////////////////////////////////////////////////////////////////////
//                            Command Pool
////////////////////////////////////////////////////////////////////////////////

#define Assert_Command_Pool()              \
{                                          \
  assert(_device       != VK_NULL_HANDLE); \
  assert(_command_pool != VK_NULL_HANDLE); \
}

void CommandPool::init(VkDevice device, uint32_t graphics_queue_index)
{
  _device = device;

  VkCommandPoolCreateInfo info
  {
    .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = graphics_queue_index,
  };
  throw_if(vkCreateCommandPool(_device, &info, nullptr, &_command_pool) != VK_SUCCESS,
           "failed to create command pool");
}

void CommandPool::destroy()
{
  Assert_Command_Pool();
  vkDestroyCommandPool(_device, _command_pool, nullptr);
}

void CommandPool::create_buffers(VkCommandBuffer* buffers, uint32_t count)
{
  Assert_Command_Pool();
  VkCommandBufferAllocateInfo info
  {
    .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool        = _command_pool,
    .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = count,
  };
  throw_if(vkAllocateCommandBuffers(_device, &info, buffers) != VK_SUCCESS,
           "failed to create command buffers");
}

auto CommandPool::create_buffer() -> CommandBuffer
{
  VkCommandBuffer buffer;
  create_buffers(&buffer, 1);
  return buffer;
}

auto CommandPool::create_buffers(uint32_t count) -> std::vector<CommandBuffer>
{
  std::vector<VkCommandBuffer> buffers(count);
  create_buffers(buffers.data(), count);
  return buffers
          | std::views::transform([](VkCommandBuffer buf) { return CommandBuffer(buf); })
          | std::ranges::to<std::vector<CommandBuffer>>();
}

void CommandPool::free_buffer(CommandBuffer& buffer)
{
  Assert_Command_Pool();
  vkFreeCommandBuffers(_device, _command_pool, 1, buffer.get_pointer());
}


////////////////////////////////////////////////////////////////////////////////
//                            Command Buffer
////////////////////////////////////////////////////////////////////////////////

#define Assert_Command_Buffer()              \
{                                            \
  assert(_command_buffer != VK_NULL_HANDLE); \
}

auto CommandBuffer::begin(usage usage) -> CommandBuffer&
{
  Assert_Command_Buffer();
  VkCommandBufferBeginInfo info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
  if (usage == usage::one_time_submit)
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(_command_buffer, &info);
  return *this;
}

auto CommandBuffer::end() -> CommandBuffer&
{
  Assert_Command_Buffer();
  vkEndCommandBuffer(_command_buffer);
  return *this;
}

auto CommandBuffer::reset() -> CommandBuffer&
{
  Assert_Command_Buffer();
  vkResetCommandBuffer(_command_buffer, 0);
  return *this;
}

} }
