#include "tk/GraphicsEngine/FrameResources.hpp"
#include "tk/GraphicsEngine/config.hpp"
#include "tk/util.hpp"

#include <cassert>

namespace tk { namespace graphics_engine {

////////////////////////////////////////////////////////////////////////////////
///                         Vertex Buffer
////////////////////////////////////////////////////////////////////////////////

void FramesDynamicBuffer::init(FrameResources* frame_resources, MemoryAllocator* alloc)
{
  _frame_resources = frame_resources;
  _alloc           = alloc;
  _byte_size_pre_frame = util::align_size(config()->buffer_size, 8);
  _buffer = alloc->create_buffer(_byte_size_pre_frame * frame_resources->size(), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
}

void FramesDynamicBuffer::destroy()
{
  _buffer.destroy();
}

auto FramesDynamicBuffer::get_current_frame_byte_offset() const -> uint32_t
{
  throw_if(_state != State::uploaded, "[FramesDynamicBuffer] cannot get frame byte offset after upload");
  return _current_frame_byte_offset;
}

void FramesDynamicBuffer::upload()
{
  // exceed capacity, need to expand
  if (_current_frame_data.size() > _byte_size_pre_frame)
  {
    _byte_size_pre_frame = util::align_size(std::max(_current_frame_data.size(), static_cast<size_t>(_byte_size_pre_frame * config()->buffer_expand_ratio)), 8);
    auto tmp_buf = _alloc->create_buffer(_byte_size_pre_frame * _frame_resources->size(), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    // add destructor for old buffer
    _frame_resources->push_old_resource([buf = this->_buffer] { buf.destroy(); });

    // reset new allocation information
    _buffer.set_realloc_info(tmp_buf);
    // update byte offset of current frame with new byte size per frame
    _current_frame_byte_offset = _frame_resources->get_current_frame_index() * _byte_size_pre_frame;
    // re-upload
    upload();
    return;
  }
  memcpy(static_cast<std::byte*>(_buffer.data()) + _current_frame_byte_offset, _current_frame_data.data(), _current_frame_data.size());
  _current_frame_data.clear();
  _state = State::uploaded;
}
  
// set state is promise the handle and address is valid
// append_range maybe recreate a bigger buffer to make handle and address invalid
auto FramesDynamicBuffer::get_handle_and_address() const -> std::pair<VkBuffer, VkDeviceAddress>
{
  throw_if(_state != State::uploaded, "[FramesDynamicBuffer] must be uploaded, then handle and address will be valid");
  return { _buffer.handle(), _buffer.address() + _current_frame_byte_offset };
}

auto FramesDynamicBuffer::size() const -> uint32_t
{
  throw_if(_state != State::append_data, "[FramesDynamicBuffer] cannot get current frame data size after upload");
  return _current_frame_data.size();
}

void FramesDynamicBuffer::frame_begin()
{
  _current_frame_data.clear();
  _current_frame_byte_offset = _byte_size_pre_frame * _frame_resources->get_current_frame_index();
  _state = State::append_data;
}

////////////////////////////////////////////////////////////////////////////////
///                         Frame Resources
////////////////////////////////////////////////////////////////////////////////

void FrameResources::init(VkDevice device, CommandPool& cmd_pool, Swapchain* swapchain)
{
  _device    = device;
  _swapchain = swapchain;

  // resize frames and submit semaphores
  _frames.resize(swapchain->size());
  _submit_sems.resize(swapchain->size());

  // create commands
  auto cmds = cmd_pool.create_commands(swapchain->size());

  // create fence and semaphore create infos
  auto fence_create_info = VkFenceCreateInfo
  {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };
  auto sem_create_info = VkSemaphoreCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

  // assign commands and create sync objects
  for (auto i = 0; i < swapchain->size(); ++i)
  {
    auto& frame = _frames[i];
    frame.cmd = cmds[i];
    throw_if(vkCreateFence(device, &fence_create_info, nullptr, &frame.fence)         != VK_SUCCESS ||
             vkCreateSemaphore(device, &sem_create_info, nullptr, &frame.acquire_sem) != VK_SUCCESS,
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
    auto& frame = _frames[i];
    vkDestroyFence(_device, frame.fence, nullptr);
    vkDestroySemaphore(_device, frame.acquire_sem, nullptr);
    vkDestroySemaphore(_device, _submit_sems[i], nullptr);
  }
  while (!_destructors.empty())
  {
    _destructors.front()(0, true);
    _destructors.pop();
  }
}

auto FrameResources::acquire_swapchain_image(bool wait) -> bool
{
  // get current frame resource
  auto& frame = _frames[_frame_index];

  // wait current frame resource fence usable
  if(vkWaitForFences(_device, 1, &frame.fence, VK_TRUE, wait ? UINT_MAX : 0) != VK_SUCCESS)
    return false;
  throw_if(vkResetFences(_device, 1, &frame.fence) != VK_SUCCESS,
           "failed to reset fence");

  // acquire usable swapchain image
  auto res = vkAcquireNextImageKHR(_device, _swapchain->get(), UINT64_MAX, frame.acquire_sem, VK_NULL_HANDLE, &_submit_sem_index);
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

void FrameResources::present_swapchain_image(VkQueue graphics_queue, VkQueue present_queue)
{
  // get current frame resource and submit semaphore
  auto& frame      = _frames[_frame_index];
  auto  submit_sem = _submit_sems[_submit_sem_index];

  // set image layout of swapchain image
  _swapchain->image(_submit_sem_index).set_layout(frame.cmd, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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
  auto swapchain = _swapchain->get();
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
  _frame_index = ++_frame_index % _swapchain->size();
}

void FrameResources::destroy_old_resources()
{
  if (!_destructors.empty() && _destructors.front()(_frame_index, false))
    _destructors.pop();
}

}}