#include "GraphicsEngine.hpp"
#include "ShaderStructs.hpp"
#include "ErrorHandling.hpp"
#include "constant.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

namespace tk
{

void GraphicsEngine::run()
{
  while (!_window.is_closed())
  {
    Window::process_events();
    update();
    draw();
  }

  vkDeviceWaitIdle(_device);
}

void GraphicsEngine::update()
{
  static auto start_time   = std::chrono::high_resolution_clock::now();
  auto        current_time = std::chrono::high_resolution_clock::now();
  float       time         = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

  UniformBufferObject ubo;
  ubo.model = glm::mat4(1.f);
  // ubo.view = glm::mat4(1.f);
  ubo.proj = glm::mat4(1.f);
  // ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.view = glm::lookAt(glm::vec3(.0f, .0f, -1.f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, .0f));
  // ubo.proj = glm::perspective(45.0f, _swapchain_image_extent.width / (float) _swapchain_image_extent.height, 0.1f, 10.0f);
  // ubo.view  = glm::lookAt(glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
  // ubo.proj  = glm::ortho(-1.f, 1.f, -1.f, 1.f, -1.f, 100.f);

  // TODO: use vma to presently mapped, and vma's copy memory function
  vmaCopyMemoryToAllocation(_vma_allocator, &ubo, _uniform_buffer_allocations[_current_frame], 0, sizeof(ubo));
}

void GraphicsEngine::draw()
{
  // TODO: use frame resources to replace every xxx[_current_frame]

  vkWaitForFences(_device, 1, &_in_flight_fences[_current_frame], VK_TRUE, UINT64_MAX);

  uint32_t image_index;
  throw_if(vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, _image_available_semaphores[_current_frame], VK_NULL_HANDLE, &image_index) != VK_SUCCESS,
           "failed to acquire swap chain image");

  vkResetFences(_device, 1, &_in_flight_fences[_current_frame]);

  vkResetCommandBuffer(_command_buffers[_current_frame], 0);
  record_command_buffer(_command_buffers[_current_frame], image_index);

  VkSemaphore wait_sems[] = { _image_available_semaphores[_current_frame] };
  VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  VkSemaphore signal_sems[] = { _render_finished_semaphores[_current_frame] };
  VkSubmitInfo info
  {
    .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount   = 1,
    .pWaitSemaphores      = wait_sems,
    .pWaitDstStageMask    = wait_stages,
    .commandBufferCount   = 1,
    .pCommandBuffers      = &_command_buffers[_current_frame],
    .signalSemaphoreCount = 1,
    .pSignalSemaphores    = signal_sems,
  };
  throw_if(vkQueueSubmit(_graphics_queue, 1, &info, _in_flight_fences[_current_frame]) != VK_SUCCESS,
           "failed to submit command buffer");

  VkPresentInfoKHR presentation_info
  {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores    = signal_sems,
    .swapchainCount     = 1,
    .pSwapchains        = &_swapchain,
    .pImageIndices      = &image_index,
  };
  throw_if(vkQueuePresentKHR(_present_queue, &presentation_info) != VK_SUCCESS,
           "failed to present swapchain image");

  _current_frame = ++_current_frame % Max_Frame_Number;
}
    
void GraphicsEngine::record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index)
{
  VkCommandBufferBeginInfo begin
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };
  throw_if(vkBeginCommandBuffer(command_buffer, &begin) != VK_SUCCESS,
           "failed to begin command buffer");

  VkClearValue clear
  {
    (float)32/255,
    (float)33/255,
    (float)36/255,
    1.f,
  };
  VkRenderPassBeginInfo render_pass_begin_info
  {
    .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass  = _render_pass,
    .framebuffer = _frame_buffers[image_index],
    .renderArea  =
    {
      .offset = { 0, 0 },
      .extent = _swapchain_image_extent,
    },
    .clearValueCount = 1,
    .pClearValues    = &clear,
  };
  vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

  VkViewport viewport
  {
    .width    = (float)_swapchain_image_extent.width,
    .height   = (float)_swapchain_image_extent.height,
    .maxDepth = 1.f,
  };
  vkCmdSetViewport(command_buffer, 0, 1, &viewport);

  VkRect2D scissor
  {
    .offset = { 0, 0 },
    .extent = _swapchain_image_extent,
  };
  vkCmdSetScissor(command_buffer, 0, 1, &scissor);

  VkDeviceSize offsets[] = { 0 };
  vkCmdBindVertexBuffers(command_buffer, 0, 1, &_vertex_buffer, offsets);
  vkCmdBindIndexBuffer(command_buffer, _index_buffer, 0, VK_INDEX_TYPE_UINT16);

  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline_layout, 0, 1, &_descriptor_sets[_current_frame], 0, nullptr);

  vkCmdDrawIndexed(command_buffer, (uint32_t)Indices.size(), 1, 0, 0, 0);

  vkCmdEndRenderPass(command_buffer);

  throw_if(vkEndCommandBuffer(command_buffer) != VK_SUCCESS,
           "failed to end command buffer");
}

}
