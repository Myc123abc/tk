#include "tk/GraphicsEngine/GraphicsEngine.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/GraphicsEngine/vk_extension.hpp"

#include <SDL3/SDL_events.h>
#include <glm/gtc/matrix_transform.hpp>

namespace tk { namespace graphics_engine {

// FIX: tmp way
static uint32_t image_index = 0;

// HACK: currently, not use
// use independent image to draw, and copy it to swapchain image has may resons,
// detail reference: https://vkguide.dev/docs/new_chapter_2/vulkan_new_rendering/#new-draw-loop
void GraphicsEngine::render_begin()
{
  //
  // get current frame resource
  //
  auto frame = get_current_frame();

  //
  // wait commands completely submitted to GPU,
  // and we can record next frame's commands.
  //

  // set forth parameter to 0, make vkWaitForFences return immediately,
  // so you can know whether finished for commands handled by GPU.
  throw_if(vkWaitForFences(_device, 1, &frame.fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS,
           "failed to wait fence");
  throw_if(vkResetFences(_device, 1, &frame.fence) != VK_SUCCESS,
           "failed to reset fence");

  //
  // acquire an available image which GPU not used currently,
  // so we can save render result on it.
  //
  auto res = vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, frame.image_available_sem, VK_NULL_HANDLE, &image_index);
  if (res == VK_ERROR_OUT_OF_DATE_KHR)
    return;
  else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
    throw_if(true, "failed to acquire swapechain image");

  //
  // now we know current frame resource is available,
  // and we need to reset command buffer to begin to record commands.
  // after record we need to end command so it can be submitted to queue.
  //
  throw_if(vkResetCommandBuffer(frame.cmd, 0) != VK_SUCCESS,
           "failed to reset command buffer");
  VkCommandBufferBeginInfo beg_info
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  vkBeginCommandBuffer(frame.cmd, &beg_info);

  // depth_image_barrier_begin(frame.cmd);

  // transition image layout to writeable
  transition_image_layout(frame.cmd, _msaa_image.handle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  transition_image_layout(frame.cmd, _resolved_image.handle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  // transition_image_layout(frame.cmd, _swapchain_images[image_index].handle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  // transition_image_layout(frame.cmd, _msaa_depth_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
  // transition_image_layout(frame.cmd, frame.depth_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

  // TODO: clear edges and blend images

  //
  // dynamic rendering
  //
  VkRenderingAttachmentInfo color_attachment
  {
    .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .imageView          = _msaa_image.view,
    .imageLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .resolveMode        = VK_RESOLVE_MODE_AVERAGE_BIT,
    .resolveImageView   = _resolved_image.view,
    .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .loadOp             = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .storeOp            = VK_ATTACHMENT_STORE_OP_STORE,
  };
  // VkRenderingAttachmentInfo depth_attachment
  // {
  //   .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
  //   .imageView          = _msaa_depth_image.view,
  //   .imageLayout        = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
  //   // HACK: when I test this on renderdoc, it not do resolve depth image...
  //   .resolveMode        = VK_RESOLVE_MODE_AVERAGE_BIT,
  //   .resolveImageView   = frame.depth_image.view,
  //   .resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
  //   .loadOp             = VK_ATTACHMENT_LOAD_OP_CLEAR,
  //   .storeOp            = VK_ATTACHMENT_STORE_OP_DONT_CARE,
  // };

  VkExtent2D extent = { _swapchain_images[image_index].extent.width, _swapchain_images[image_index].extent.height };

  VkRenderingInfo rendering
  {
    .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
    .renderArea           =
    {
      .extent = extent,
    },
    .layerCount           = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments    = &color_attachment,
    // .pDepthAttachment     = &depth_attachment,
  };
  vkCmdBeginRendering(frame.cmd, &rendering);

  VkViewport viewport
  {
    .width  = (float)extent.width,
    .height = (float)extent.height, 
    .maxDepth = 1.f,
  };
  vkCmdSetViewport(frame.cmd, 0, 1, &viewport);
  VkRect2D scissor 
  {
    .extent = extent,
  };
  vkCmdSetScissor(frame.cmd, 0, 1, &scissor);

  vkCmdBindPipeline(frame.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2D_pipeline);
}

void GraphicsEngine::render_end()
{
  //
  // get current frame resource
  //
  auto frame = get_current_frame();

  vkCmdEndRendering(frame.cmd);

  post_process();

  VkExtent2D extent = { _swapchain_images[image_index].extent.width, _swapchain_images[image_index].extent.height };

  // TODO: need resolved_image? directly use smaa to swapchain image is ok so...

  // copy resolved image to swapchain image
  // FIXME: can del
  transition_image_layout(frame.cmd, _resolved_image.handle, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  transition_image_layout(frame.cmd, _blend_image.handle, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  transition_image_layout(frame.cmd, _edges_image.handle, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  transition_image_layout(frame.cmd, _swapchain_images[image_index].handle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  copy_image(frame.cmd, _resolved_image.handle, _swapchain_images[image_index].handle, extent, extent);

  // transition image layout to presentable
  transition_image_layout(frame.cmd, _swapchain_images[image_index].handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  // depth_image_barrier_end(frame.cmd);

  throw_if(vkEndCommandBuffer(frame.cmd) != VK_SUCCESS,
           "failed to end command buffer");

  //
  // submit commands to queue, which will copied to GPU.
  //
  VkCommandBufferSubmitInfo cmd_submit_info
  {
    .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
    .commandBuffer = frame.cmd,
  };
  VkSemaphoreSubmitInfo wait_sem_submit_info
  {
    .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
    .semaphore = frame.image_available_sem,
    .value     = 1,
    .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
  };
  auto signal_sem_submit_info      = wait_sem_submit_info;
  signal_sem_submit_info.semaphore = frame.render_finished_sem;
  signal_sem_submit_info.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

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
  throw_if(vkQueueSubmit2(_graphics_queue, 1, &submit_info, frame.fence),
           "failed to submit to queue");

  //
  // present to screen
  //
  VkPresentInfoKHR presentation_info
  {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores    = &frame.render_finished_sem,
    .swapchainCount     = 1,
    .pSwapchains        = &_swapchain,
    .pImageIndices      = &image_index,
  };
  auto res = vkQueuePresentKHR(_present_queue, &presentation_info); 
  if (res != VK_SUCCESS               &&
      res != VK_ERROR_OUT_OF_DATE_KHR &&
      res != VK_SUBOPTIMAL_KHR)
    throw_if(true, "failed to present swapchain image");

  // update frame index
  _current_frame = ++_current_frame % _swapchain_images.size();
}

/*
 * buffer storage
 * | vertices | indices |
 */
void GraphicsEngine::render(std::span<IndexInfo> index_infos, glm::vec2 const& window_extent, glm::vec2 const& display_pos)
{
  auto cmd = _frames[_current_frame].cmd;

  PushConstant pc
  {
    .vertices      = _buffer.address(),
    .window_extent = window_extent,
    .display_pos   = display_pos,
  };
  vkCmdPushConstants(cmd, _2D_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);

  for (auto const& index_info : index_infos)
    vkCmdDrawIndexed(cmd, index_info.count, 1, index_info.offset, 0, 0);
}

void GraphicsEngine::update(std::span<Vertex> vertices, std::span<uint16_t> indices)
{
  auto vertices_size = sizeof(Vertex)   * vertices.size();
  auto indices_size  = sizeof(uint16_t) * indices.size();

  throw_if(vertices_size + indices_size > 2 * 1024 * 1024, "too big vertices indices data!(bigger than 2MB)");

  throw_if(vmaCopyMemoryToAllocation(_mem_alloc.get(), vertices.data(), _buffer.allocation(), 0, vertices_size) != VK_SUCCESS,
           "failed to copy vertices data to buffer");
  throw_if(vmaCopyMemoryToAllocation(_mem_alloc.get(), indices.data(), _buffer.allocation(), vertices_size, indices_size) != VK_SUCCESS,
           "failed to copy indices data to buffer");
           
  vkCmdBindIndexBuffer(_frames[_current_frame].cmd, _buffer.handle(), vertices_size, VK_INDEX_TYPE_UINT16);
}

void clear(Command const& cmd, Image const& img)
{
  VkImageSubresourceRange range
  {
    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .levelCount = VK_REMAINING_MIP_LEVELS,
    .layerCount = VK_REMAINING_ARRAY_LAYERS,
  };
  VkClearColorValue value{};
  vkCmdClearColorImage(cmd, img.handle, VK_IMAGE_LAYOUT_GENERAL, &value, 1, &range);
}

void GraphicsEngine::post_process()
{
  auto frame = get_current_frame();

  VkExtent2D extent = { _swapchain_images[image_index].extent.width, _swapchain_images[image_index].extent.height };

  //
  // edge detection
  //
  transition_image_layout(frame.cmd, _edges_image.handle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
  clear(frame.cmd, _edges_image);
  transition_image_layout(frame.cmd, _resolved_image.handle, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  
  vkCmdBindPipeline(frame.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _smaa_pipeline[0]);

  uint32_t width, height;
  _window->get_framebuffer_size(width, height);

  PushConstant_SMAA pc
  {
    .smaa_rt_metrics = glm::vec4(1.f / width, 1.f / height, width, height),
  };
  vkCmdPushConstants(frame.cmd, _smaa_pipeline[0].get_layout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);

  // HACK: all pass use same pipeline layout, but pipeline layout will be discard?
  bind_descriptor_buffer(frame.cmd, _descriptor_buffer.address(), VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT, _smaa_pipeline_layout, VK_PIPELINE_BIND_POINT_COMPUTE);

  vkCmdDispatch(frame.cmd, std::ceil((extent.width + 15) / 16), std::ceil((extent.height + 15) / 16), 1);

  //
  // blend weight
  //
  vkCmdBindPipeline(frame.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _smaa_pipeline[1]);
  transition_image_layout(frame.cmd, _blend_image.handle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
  clear(frame.cmd, _blend_image);
  transition_image_layout(frame.cmd, _edges_image.handle, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  vkCmdDispatch(frame.cmd, std::ceil((extent.width + 15) / 16), std::ceil((extent.height + 15) / 16), 1);

  //
  // neighbor
  //
  vkCmdBindPipeline(frame.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _smaa_pipeline[2]);
  transition_image_layout(frame.cmd, _smaa_image.handle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
  transition_image_layout(frame.cmd, _blend_image.handle, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  vkCmdDispatch(frame.cmd, std::ceil((extent.width + 15) / 16), std::ceil((extent.height + 15) / 16), 1);
}
    
} }
