//
// command pool
//
// initialize VkCommandPool and create command buffers
//

#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <cassert>

namespace tk
{

  class CommandBuffer;

  class CommandPool
  {
  public:
    CommandPool()  = default;
    ~CommandPool() = default;

    CommandPool(CommandPool const&)            = delete;
    CommandPool(CommandPool&&)                 = delete;
    CommandPool& operator=(CommandPool const&) = delete;
    CommandPool& operator=(CommandPool&&)      = delete;

    void init(VkDevice device, uint32_t graphics_queue_index);
    void destroy();

    // HACK: not should be create single buffer,
    // just like suballoc and signle buffer.
    // create all command buffers together,
    // and use them on draw or one time command
    auto create_buffer()                -> CommandBuffer;
    auto create_buffers(uint32_t count) -> std::vector<CommandBuffer>;

    // HACK: like create_buffer
    void free_buffer(CommandBuffer& buffer);

  private:
    void create_buffers(VkCommandBuffer* buffers, uint32_t count);

  private:
    VkDevice      _device       = VK_NULL_HANDLE;
    VkCommandPool _command_pool = VK_NULL_HANDLE;
  };

  class CommandBuffer
  {
  public:
    CommandBuffer(VkCommandBuffer buffer): _command_buffer(buffer) {}

    enum class usage
    {
      none,
      one_time_submit,
    };

    auto begin(usage usage = usage::none) -> CommandBuffer&;
    auto end()                            -> CommandBuffer&;
    auto reset()                          -> CommandBuffer&;

    auto get() { return _command_buffer; }
    auto get_pointer()
    {
      assert(_command_buffer != VK_NULL_HANDLE);
      return &_command_buffer;
    }

  private:
    VkCommandBuffer _command_buffer;
  };

}
