#include "GraphicsEngine.hpp"
#include "ErrorHandling.hpp"
#include "constant.hpp"
#include "Shape.hpp"

#include <SDL3/SDL_events.h>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

namespace tk { namespace graphics_engine {

VkClearColorValue Clear_Value;

void GraphicsEngine::update()
{
  static auto start_time   = std::chrono::high_resolution_clock::now();
  auto        current_time = std::chrono::high_resolution_clock::now();
  float       time         = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

  // clear value
  uint32_t circle = time / 3;
  auto val = std::abs(std::sin(time / 3 * M_PI));
  float r{}, g{}, b{};
  auto mod = circle % 3;
  if (mod == 0)
    r = val;
  else if (mod == 1)
    g = val;
  else
    b = val;
  Clear_Value = { { r, g, b, 1.f } };
}

// use independent image to draw, and copy it to swapchain image has may resons,
// detail reference: https://vkguide.dev/docs/new_chapter_2/vulkan_new_rendering/#new-draw-loop
void GraphicsEngine::draw()
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
  uint32_t image_index;
  auto res = vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, frame.image_available_sem, VK_NULL_HANDLE, &image_index);
  if (res == VK_ERROR_OUT_OF_DATE_KHR)
    return;
  else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
    throw_if(true, "failed to acquire swapechain image");

  // get draw extent
  _draw_extent.width  = std::min(_swapchain_image_extent.width, _image.extent.width);
  _draw_extent.height = std::min(_swapchain_image_extent.height, _image.extent.height);

  //
  // now we know current frame resource is available,
  // and we need to reset command buffer to begin to record commands.
  // after record we need to end command so it can be submitted to queue.
  //
  throw_if(vkResetCommandBuffer(frame.command_buffer, 0) != VK_SUCCESS,
           "failed to reset command buffer");
  VkCommandBufferBeginInfo beg_info
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  vkBeginCommandBuffer(frame.command_buffer, &beg_info);

  // HACK: make frame.command_buffer and _swapchain_images[image_index] to frame resource function
  // only need like as follow:
  //   render_begin();  // get current frame, available command buffer and image.
  //                    // also switch command buffer to available, and clear vlaue.
  //   some_render_ops(); 
  //   render_end();    // submit commands to queue and end everything like command buffer, etc.

  // transition image layout to writeable
  transition_image_layout(frame.command_buffer, _image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

  draw_background(frame.command_buffer);

  transition_image_layout(frame.command_buffer, _image.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  transition_image_layout(frame.command_buffer, _depth_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

  draw(frame.command_buffer);

  // copy image to swapchain image
  transition_image_layout(frame.command_buffer, _image.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  transition_image_layout(frame.command_buffer, _swapchain_images[image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  copy_image(frame.command_buffer, _image.image, _swapchain_images[image_index], _draw_extent, _swapchain_image_extent);

  // transition image layout to presentable
  transition_image_layout(frame.command_buffer, _swapchain_images[image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  throw_if(vkEndCommandBuffer(frame.command_buffer) != VK_SUCCESS,
           "failed to end command buffer");

  //
  // submit commands to queue, which will copied to GPU.
  //
  VkCommandBufferSubmitInfo cmd_submit_info
  {
    .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
    .commandBuffer = frame.command_buffer,
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
  res = vkQueuePresentKHR(_present_queue, &presentation_info); 
  if (res != VK_SUCCESS               &&
      res != VK_ERROR_OUT_OF_DATE_KHR &&
      res != VK_SUBOPTIMAL_KHR)
    throw_if(true, "failed to present swapchain image");

  // update frame index
  _current_frame = ++_current_frame % Max_Frame_Number;
}

void GraphicsEngine::draw_background(Command cmd)
{
  auto clear_range = get_image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
  vkCmdClearColorImage(cmd, _image.image, VK_IMAGE_LAYOUT_GENERAL, &Clear_Value, 1, &clear_range);
}

// HACK:
// temporary render begin function, it should also have reset command, image transoform, etc. 
// and also need to abstract VkCommandBuffer
// also view and extent mostly is fixed
void render_begin(VkCommandBuffer cmd, VkImageView view, VkExtent2D extent)
{
  VkRenderingAttachmentInfo attachment
  {
    .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .imageView   = view,
    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .loadOp      = VK_ATTACHMENT_LOAD_OP_LOAD,
    .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
  };
  VkRenderingInfo rendering
  {
    .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
    .renderArea           =
    {
      .extent = extent,
    },
    .layerCount           = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments    = &attachment,
  };
  vkCmdBeginRendering(cmd, &rendering);

  VkViewport viewport
  {
    .width  = (float)extent.width,
    .height = (float)extent.height, 
    .maxDepth = 1.f,
  };
  vkCmdSetViewport(cmd, 0, 1, &viewport);
  VkRect2D scissor 
  {
    .extent = extent,
  };
  vkCmdSetScissor(cmd, 0, 1, &scissor);
}

// HACK: for consistence with render_begin, and need to as interface in VkCommandBuffer abstract
void render_end(VkCommandBuffer cmd)
{
  vkCmdEndRendering(cmd);
}

void GraphicsEngine::draw(VkCommandBuffer cmd)
{
  render_begin(cmd, _image.view, {_draw_extent.width, _draw_extent.height});

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _2D_pipeline);

  ShapeInfo info
  {
    .model    = glm::mat4(1.f),
    .vertices = _mesh_buffer.address,
  };
  vkCmdPushConstants(cmd, _2D_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(info), &info);

  vkCmdBindIndexBuffer(cmd, _mesh_buffer.indices.handle, 0, VK_INDEX_TYPE_UINT8);
  vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);

  render_end(cmd);
}
    
} }
