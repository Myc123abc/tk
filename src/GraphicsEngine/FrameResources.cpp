#include "tk/GraphicsEngine/FrameResources.hpp"
#include "tk/ErrorHandling.hpp"

#include <cassert>

namespace tk { namespace graphics_engine {

void FrameResources::init(VkDevice device, CommandPool& cmd_pool, uint32_t size)
{
  _device = device;

  // resize frames and submit semaphores
  _frames.resize(size);
  _submit_sems.resize(size);

  // create commands
  auto cmds = cmd_pool.create_commands(size);

  // create fence and semaphore create infos
  auto fence_create_info = VkFenceCreateInfo
  {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };
  auto sem_create_info = VkSemaphoreCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

  // assign commands and create sync objects
  for (auto i = 0; i < size; ++i)
  {
    _frames[i].cmd = cmds[i];
    throw_if(vkCreateFence(device, &fence_create_info, nullptr, &_frames[i].fence)         != VK_SUCCESS ||
             vkCreateSemaphore(device, &sem_create_info, nullptr, &_frames[i].acquire_sem) != VK_SUCCESS,
             "failed to create sync objects");
    throw_if(vkCreateSemaphore(device, &sem_create_info, nullptr, &_submit_sems[i]) != VK_SUCCESS, 
             "failed to create semaphore");
  }
}

void FrameResources::destroy()
{
  assert(_frames.size() == _submit_sems.size());
  for (auto i = 0; i < _frames.size(); ++i)
  {
    vkDestroyFence(_device, _frames[i].fence, nullptr);
    vkDestroySemaphore(_device, _frames[i].acquire_sem, nullptr);
    vkDestroySemaphore(_device, _submit_sems[i], nullptr);
  }
}

auto FrameResources::acquire_swapchain_image(VkSwapchainKHR swapchain, bool wait) -> bool
{
  // get current frame resource
  auto& frame = _frames[_frame_index];

  // wait current frame resource fence usable
  if(vkWaitForFences(_device, 1, &frame.fence, VK_TRUE, wait ? UINT_MAX : 0) != VK_SUCCESS)
    return false;
  throw_if(vkResetFences(_device, 1, &frame.fence) != VK_SUCCESS,
           "failed to reset fence");

  // acquire usable swapchain image
  auto res = vkAcquireNextImageKHR(_device, swapchain, UINT64_MAX, frame.acquire_sem, VK_NULL_HANDLE, &_submit_sem_index);
  if (res == VK_ERROR_OUT_OF_DATE_KHR)
    return false;
  else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
    throw_if(true, "failed to acquire swapechain image");

  // reset command
  throw_if(vkResetCommandBuffer(frame.cmd, 0) != VK_SUCCESS,
           "failed to reset command buffer");

  // start to frame command
  auto command_begin_info = VkCommandBufferBeginInfo
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  vkBeginCommandBuffer(frame.cmd, &command_begin_info);

  return true;
}

void FrameResources::present_swapchain_image(VkSwapchainKHR swapchain, VkQueue graphics_queue, VkQueue present_queue, std::span<Image> swapchain_images)
{
  // get current frame resource and submit semaphore
  auto& frame      = _frames[_frame_index];
  auto  submit_sem = _submit_sems[_submit_sem_index];

  // set image layout of swapchain image
  swapchain_images[_submit_sem_index].set_layout(frame.cmd, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  // finish frameing command
  throw_if(vkEndCommandBuffer(frame.cmd) != VK_SUCCESS,
           "failed to end command buffer");

  // command submit info
  VkCommandBufferSubmitInfo cmd_submit_info
  {
    .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
    .commandBuffer = frame.cmd,
  };

  // wait semaphore submit info
  VkSemaphoreSubmitInfo wait_sem_submit_info
  {
    .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
    .semaphore = frame.acquire_sem,
    .value     = 1,
    .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
  };

  // signal semaphore submit info
  VkSemaphoreSubmitInfo signal_sem_submit_info
  {
    .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
    .semaphore = submit_sem,
    .value     = 1,
    .stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
  };

  // submit info
  VkSubmitInfo2 submit_info
  {
    .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
    .waitSemaphoreInfoCount   = 1,
    .pWaitSemaphoreInfos      = &wait_sem_submit_info,
    .commandBufferInfoCount   = 1,
    .pCommandBufferInfos      = &cmd_submit_info,
    .signalSemaphoreInfoCount = 1,
    .pSignalSemaphoreInfos    = &signal_sem_submit_info,
  };

  // queue submit
  throw_if(vkQueueSubmit2(graphics_queue, 1, &submit_info, frame.fence),
           "failed to submit to queue");

  // present info
  VkPresentInfoKHR present_info
  {
    .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores    = &submit_sem,
    .swapchainCount     = 1,
    .pSwapchains        = &swapchain,
    .pImageIndices      = &_submit_sem_index,
  };

  // present swapchain image
  auto res = vkQueuePresentKHR(present_queue, &present_info); 
  if (res != VK_SUCCESS               &&
      res != VK_ERROR_OUT_OF_DATE_KHR &&
      res != VK_SUBOPTIMAL_KHR)
    throw_if(true, "failed to present swapchain image");

  // update next frame frame resource index
  _frame_index = ++_frame_index % swapchain_images.size();
}

}}