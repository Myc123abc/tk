#include "GraphicsEngine.hpp"
#include "ShaderStructs.hpp"
#include "ErrorHandling.hpp"
#include "constant.hpp"

#include <SDL3/SDL_events.h>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui_impl_vulkan.h>
#include <imgui_impl_sdl3.h>

#include <thread>
#include <chrono>
#include <numbers>

namespace tk { namespace graphics_engine {

void GraphicsEngine::run()
{
  bool quit  = false;
  bool pause = false;

  SDL_Event event;

  while (!quit)
  {
    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
      case SDL_EVENT_QUIT:
        quit = true;
        break;
      case SDL_EVENT_WINDOW_MINIMIZED:
        pause = true;
        break;
      case SDL_EVENT_WINDOW_MAXIMIZED:
        pause = false;
        break;
      }
    }

    if (!pause)
    {
      update();
      draw();

      // ImGui_ImplVulkan_NewFrame();
      // ImGui_ImplSDL3_NewFrame();
      // ImGui::NewFrame();
      //
      // ImGui::Render();
      // ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), );
    }
    else
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

VkClearColorValue Clear_Value;

void GraphicsEngine::update()
{
  static auto start_time   = std::chrono::high_resolution_clock::now();
  auto        current_time = std::chrono::high_resolution_clock::now();
  float       time         = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

  UniformBufferObject ubo;
  ubo.model = glm::mat4(1.f);
  ubo.view = glm::mat4(1.f);
  ubo.proj = glm::mat4(1.f);
  // ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  // ubo.view = glm::lookAt(glm::vec3(.0f, .0f, -1.f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, .0f));
  // ubo.proj = glm::perspective(45.0f, _swapchain_image_extent.width / (float) _swapchain_image_extent.height, 0.1f, 10.0f);
  // ubo.view  = glm::lookAt(glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
  // ubo.proj  = glm::ortho(-1.f, 1.f, -1.f, 1.f, -1.f, 100.f);

  // TODO: use vma to presently mapped, and vma's copy memory function
  // vmaCopyMemoryToAllocation(_vma_allocator, &ubo, _uniform_buffer_allocations[_current_frame], 0, sizeof(ubo));

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

//
// use independent image to draw, and copy it to swapchain image has may resons,
// detail reference: https://vkguide.dev/docs/new_chapter_2/vulkan_new_rendering/#new-draw-loop
//
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
  throw_if(vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, frame.image_available_sem, VK_NULL_HANDLE, &image_index) != VK_SUCCESS,
           "failed to acquire swap chain image");

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
  draw_geometry(frame.command_buffer);

  // copy image to swapchain image
  transition_image_layout(frame.command_buffer, _image.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  transition_image_layout(frame.command_buffer, _swapchain_images[image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  VkExtent2D extent { _image.extent.width, _image.extent.height };
  copy_image(frame.command_buffer, _image.image, _swapchain_images[image_index], extent, _swapchain_image_extent);

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
  throw_if(vkQueuePresentKHR(_present_queue, &presentation_info) != VK_SUCCESS,
           "failed to present swapchain image");

  // update frame index
  _current_frame = ++_current_frame % Max_Frame_Number;
}

void GraphicsEngine::draw_background(VkCommandBuffer cmd)
{
  int i = 1;

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _compute_pipeline[i]);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _compute_pipeline_layout[i], 0, 1, &_descriptor_set, 0, nullptr);

  if (i != 0)
  {
    PushContant pc;
    pc.data1 = glm::vec4(1, 0, 0, 1);
    pc.data2 = glm::vec4(0, 0, 1, 1);
    vkCmdPushConstants(cmd, _compute_pipeline_layout[i], VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);
  }

  vkCmdDispatch(cmd, std::ceil(_swapchain_image_extent.width / 16.f), std::ceil(_swapchain_image_extent.height / 16.f), 1);
  // auto clear_range = get_image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
  // vkCmdClearColorImage(cmd, _image.image, VK_IMAGE_LAYOUT_GENERAL, &Clear_Value, 1, &clear_range);
}

void GraphicsEngine::draw_geometry(VkCommandBuffer cmd)
{
  VkRenderingAttachmentInfo attachment
  {
    .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .imageView   = _image.view,
    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .loadOp      = VK_ATTACHMENT_LOAD_OP_LOAD,
    .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
  };
  VkRenderingInfo rendering
  {
    .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
    .renderArea           =
    {
      .extent = { _image.extent.width , _image.extent.height },
    },
    .layerCount           = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments    = &attachment,
  };
  vkCmdBeginRendering(cmd, &rendering);

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphics_pipeline);

  VkViewport viewport
  {
    .width  = (float)_image.extent.width,
    .height = (float)_image.extent.height, 
    .maxDepth = 1.f,
  };
  vkCmdSetViewport(cmd, 0, 1, &viewport);
  VkRect2D scissor 
  {
    .extent =
    {
      .width  = _image.extent.width,
      .height = _image.extent.height, 
    },
  };
  vkCmdSetScissor(cmd, 0, 1, &scissor);

  vkCmdDraw(cmd, 3, 1, 0, 0);

  vkCmdEndRendering(cmd);
}
    
} }
