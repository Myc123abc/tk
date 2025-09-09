#include "CommandPool.hpp"
#include "../ErrorHandling.hpp"
#include <cassert>

namespace tk { namespace graphics_engine {

////////////////////////////////////////////////////////////////////////////////
//                            Command  Pool
////////////////////////////////////////////////////////////////////////////////

void CommandPool::init(VkDevice device, uint32_t queue_index)
{
  _device = device;
  VkCommandPoolCreateInfo info
  {
    .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = queue_index,
  };
  throw_if(vkCreateCommandPool(device, &info, nullptr, &_pool) != VK_SUCCESS,
           "failed to create command pool");
}

void CommandPool::destroy()
{
  if (_pool != VK_NULL_HANDLE)
    vkDestroyCommandPool(_device, _pool, nullptr);
  _device = VK_NULL_HANDLE;
  _pool   = VK_NULL_HANDLE;
}

auto CommandPool::create_commands(uint32_t count) -> std::vector<Command>
{
  auto cmds = std::vector<VkCommandBuffer>(count);
  VkCommandBufferAllocateInfo info
  {
    .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool        = _pool,
    .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = count,
  };
  throw_if(vkAllocateCommandBuffers(_device, &info, cmds.data()) != VK_SUCCESS,
           "failed to create command buffers");
  return { cmds.begin(), cmds.end() };
}

auto CommandPool::create_command() -> Command
{
  return std::move(create_commands(1)[0]);
}


////////////////////////////////////////////////////////////////////////////////
//                               Command
////////////////////////////////////////////////////////////////////////////////

auto Command::begin(VkCommandBufferUsageFlags flags) -> Command&
{
  assert(_command);
  VkCommandBufferBeginInfo beg_info
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = flags,
  };
  vkBeginCommandBuffer(_command, &beg_info);
  return *this;
}

auto Command::end() -> Command&
{
  assert(_command);
  vkEndCommandBuffer(_command);
  return *this;
}

void Command::submit_wait_free(CommandPool& pool, VkQueue queue)
{
  assert(_command);
  VkSubmitInfo info
  {
    .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers    = &_command,
  };
  throw_if(vkQueueSubmit(queue, 1, &info, VK_NULL_HANDLE) != VK_SUCCESS,
           "failed to submit by queue");
  throw_if(vkQueueWaitIdle(queue) != VK_SUCCESS,
           "failed to wait queue");
  vkFreeCommandBuffers(pool._device, pool._pool, 1, &_command);
}

}}
