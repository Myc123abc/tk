//
// Command Pool
//
// allocate commands
//

#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace tk { namespace graphics_engine {

  class Command;
  class CommandPool
  {
    friend class Command;
  public:
    CommandPool()  = default;
    ~CommandPool() = default;

    CommandPool(CommandPool const&)            = delete;
    CommandPool(CommandPool&&)                 = delete;
    CommandPool& operator=(CommandPool const&) = delete;
    CommandPool& operator=(CommandPool&&)      = delete;

    void init(VkDevice device, uint32_t queue_index);
    void destroy();

    auto create_commands(uint32_t count) -> std::vector<Command>;
    auto create_command()                -> Command;

  private:
    VkDevice      _device = VK_NULL_HANDLE;
    VkCommandPool _pool   = VK_NULL_HANDLE; 
  };

  class Command
  {
  public:
    Command() = default;
    Command(VkCommandBuffer cmd) : _command(cmd) {}

    operator VkCommandBuffer() const { return _command; }

    auto begin(VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) -> Command&;
    auto end() -> Command&;

    /**
     * mostly use for single command after record finish.
     * commands usually will be destroied when command pool destroied.
     * will in this function you also can manual free the command.
     * @param pool command pool which allocated this command
     * @param queue submitteed queue
     * @throw std::runtime_error failed because of submit, wait submit
     */
    void submit_wait_free(CommandPool& pool, VkQueue queue);

  private:
    VkCommandBuffer _command = VK_NULL_HANDLE;
  };

}}
