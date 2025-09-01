//
// frame resources
//

#pragma once

#include "CommandPool.hpp"
#include "Swapchain.hpp"


namespace tk { namespace graphics_engine {

struct FrameResource
{
  Command     cmd;
  VkFence     fence;
  VkSemaphore acquire_sem;
  // maybe use pointer of custom
};

class FrameResources
{
public:
  FrameResources() = default;

  void init(VkDevice device, CommandPool& cmd_pool, Swapchain* swapchain);
  void destroy();

  auto& get_command() const noexcept { return _frames[_frame_index].cmd; }

  // TODO:
  // make swapchain, swapchain images and graphics queue as class Swapchain
  // as parameter with FrameResources functions
  auto acquire_swapchain_image(bool wait) -> bool;
  void present_swapchain_image(VkQueue graphics_queue, VkQueue present_queue);
  auto& get_swapchain_image() noexcept { return _swapchain->image(_submit_sem_index); }

  // TODO: vertex buffer should be a frame resource
  // this is tmp
  auto get_frame_index() const noexcept { return _frame_index; }

private:
  VkDevice                   _device;
  std::vector<FrameResource> _frames;
  std::vector<VkSemaphore>   _submit_sems;
  uint32_t                   _frame_index{};
  uint32_t                   _submit_sem_index{};
  Swapchain*                 _swapchain{};
};

}}