#include "tk/GraphicsEngine/GraphicsEngine.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/GraphicsEngine/vk_extension.hpp"
#include "tk/GraphicsEngine/config.hpp"

#include <glm/gtc/matrix_transform.hpp>


namespace tk { namespace graphics_engine {

auto get_cursor_position() -> glm::vec2;

auto GraphicsEngine::frame_begin() -> bool
{
  auto* frame = &get_current_frame();

  if(vkWaitForFences(_device, 1, &frame->fence, VK_TRUE, _wait_fence ? UINT_MAX : 0) != VK_SUCCESS)
    return false;
  throw_if(vkResetFences(_device, 1, &frame->fence) != VK_SUCCESS,
           "failed to reset fence");

  auto res = vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, frame->acquire_sem, VK_NULL_HANDLE, &_current_swapchain_image_index);
  if (res == VK_ERROR_OUT_OF_DATE_KHR)
    return false;
  else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
    throw_if(true, "failed to acquire swapechain image");
  frame->submit_sem = _submit_sems[_current_swapchain_image_index];

  throw_if(vkResetCommandBuffer(frame->cmd, 0) != VK_SUCCESS,
           "failed to reset command buffer");
  VkCommandBufferBeginInfo beg_info
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  vkBeginCommandBuffer(frame->cmd, &beg_info);

  set_pipeline_state(frame->cmd);

  if (config()->use_descriptor_buffer)
    bind_descriptor_buffer(frame->cmd, _descriptor_buffer);

  return true;
}

void GraphicsEngine::frame_end()
{
  auto& frame = get_current_frame();

  get_current_swapchain_image().set_layout(frame.cmd, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  
  throw_if(vkEndCommandBuffer(frame.cmd) != VK_SUCCESS,
           "failed to end command buffer");

  VkCommandBufferSubmitInfo cmd_submit_info
  {
    .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
    .commandBuffer = frame.cmd,
  };
  VkSemaphoreSubmitInfo wait_sem_submit_info
  {
    .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
    .semaphore = frame.acquire_sem,
    .value     = 1,
    .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
  };
  auto signal_sem_submit_info      = wait_sem_submit_info;
  signal_sem_submit_info.semaphore = frame.submit_sem;
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

  VkPresentInfoKHR presentation_info
  {
    .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores    = &frame.submit_sem,
    .swapchainCount     = 1,
    .pSwapchains        = &_swapchain,
    .pImageIndices      = &_current_swapchain_image_index,
  };
  auto res = vkQueuePresentKHR(_present_queue, &presentation_info); 
  if (res != VK_SUCCESS               &&
      res != VK_ERROR_OUT_OF_DATE_KHR &&
      res != VK_SUBOPTIMAL_KHR)
    throw_if(true, "failed to present swapchain image");

  _current_frame = ++_current_frame % _swapchain_images.size();
}

void GraphicsEngine::render_begin(Image& image)
{
  auto& cmd = get_current_frame().cmd;

  image.set_layout(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  
  auto color_attachment = VkRenderingAttachmentInfo
  {
    .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .imageView          = image.view(),
    .imageLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .loadOp             = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp            = VK_ATTACHMENT_STORE_OP_STORE,
  };
  auto rendering = VkRenderingInfo
  {
    .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
    .renderArea           = { .extent = image.extent2D(), },
    .layerCount           = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments    = &color_attachment,
  };
  vkCmdBeginRendering(cmd, &rendering);
}

void GraphicsEngine::sdf_render_begin()
{
  _text_rgba_image.set_layout(get_current_frame().cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  render_begin(get_current_swapchain_image());
}

void GraphicsEngine::set_pipeline_state(Command const& cmd)
{
  VkViewport viewport
  {
    .width  = static_cast<float>(get_current_swapchain_image().extent3D().width),
    .height = static_cast<float>(get_current_swapchain_image().extent3D().height), 
    .maxDepth = 1.f,
  };

  if (config()->use_shader_object)
  {
    graphics_engine::vkCmdSetCullModeEXT(cmd, VK_CULL_MODE_BACK_BIT);
    graphics_engine::vkCmdSetDepthWriteEnableEXT(cmd, VK_FALSE);
    vkCmdSetRasterizerDiscardEnable(cmd, VK_FALSE);
    vkCmdSetDepthTestEnable(cmd, VK_FALSE);
    vkCmdSetStencilTestEnable(cmd, VK_FALSE);
    vkCmdSetDepthBiasEnable(cmd, VK_FALSE);
    graphics_engine::vkCmdSetPolygonModeEXT(cmd, VK_POLYGON_MODE_FILL);
    graphics_engine::vkCmdSetRasterizationSamplesEXT(cmd, VK_SAMPLE_COUNT_1_BIT);
    VkSampleMask sampler_mask{ 0b1 };
    graphics_engine::vkCmdSetSampleMaskEXT(cmd, VK_SAMPLE_COUNT_1_BIT, &sampler_mask);
    vkCmdSetFrontFace(cmd, VK_FRONT_FACE_CLOCKWISE);
    graphics_engine::vkCmdSetAlphaToCoverageEnableEXT(cmd, VK_FALSE);
    VkRect2D scissor{ .extent = get_current_swapchain_image().extent2D(), };
    vkCmdSetViewportWithCount(cmd, 1, &viewport);
    vkCmdSetScissorWithCount(cmd, 1, &scissor);
    vkCmdSetPrimitiveTopology(cmd, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    vkCmdSetPrimitiveRestartEnable(cmd, VK_FALSE);
    graphics_engine::vkCmdSetVertexInputEXT(cmd, 0, nullptr, 0, nullptr);
    VkBool32 color_blend_enables{ VK_TRUE };
    graphics_engine::vkCmdSetColorBlendEnableEXT(cmd, 0, 1, &color_blend_enables);
    if (color_blend_enables)
    {
      VkColorBlendEquationEXT blend_op
      {
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp        = VK_BLEND_OP_ADD,
      };
      graphics_engine::vkCmdSetColorBlendEquationEXT(cmd, 0, 1, &blend_op);
    }
    VkColorComponentFlags color_write_mask{ VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT };
    graphics_engine::vkCmdSetColorWriteMaskEXT(cmd, 0, 1, &color_write_mask);
  }
  else
  {
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    VkRect2D scissor{ .extent = get_current_swapchain_image().extent2D(), };
    vkCmdSetScissor(cmd, 0, 1, &scissor);
  }
}

void GraphicsEngine::render_end()
{
  auto& frame = get_current_frame();
  vkCmdEndRendering(frame.cmd);
}

#if 0
void GraphicsEngine::sdf_update(std::span<glm::vec2> points, std::span<ShapeInfo> infos)
{
  _buffers[_current_frame].clear().append_range(points).append_range(infos);
}

void GraphicsEngine::sdf_render(uint32_t offset, uint32_t num)
{
  auto& cmd = get_current_frame().cmd;

  auto pc = PushConstant_SDF
  {
    .address       = _buffers[_current_frame].address(),
    // this offset is not byte offset, is float offset
    .offset        = offset,
    .num           = num,
    .window_extent = _window->get_framebuffer_size(),
  };

  _sdf_render_pipeline.bind(cmd, pc);

  vkCmdDraw(cmd, 6, 1, 0, 0);
}
#endif

void GraphicsEngine::sdf_render(std::span<Vertex> vertices, std::span<uint16_t> indices)
{
  // get buffer and clear
  auto& buffer = _buffers[_current_frame].clear();

  // upload vertices to buffer
  buffer.append_range(vertices);

  // get offset of indices
  auto offset = buffer.size();
  // upload indices to buffer
  buffer.append_range(indices);
  
  auto& cmd = get_current_frame().cmd;

  // bind index buffer
  vkCmdBindIndexBuffer(cmd, buffer.handle(), offset, VK_INDEX_TYPE_UINT16);

  auto pc = PushConstant_SDF
  {
    .address       = buffer.address(),
    .window_extent = _window->get_framebuffer_size(),
  };

  _sdf_render_pipeline.bind(cmd, pc);

  vkCmdDrawIndexed(cmd, indices.size(), 1, 0, 0, 0);
}

void GraphicsEngine::text_mask_render_begin()
{
  render_begin(_text_rgba_image);
}

auto GraphicsEngine::parse_text(std::string_view text, glm::vec2 const& pos, float size, glm::vec4 const& color) -> std::pair<glm::vec2, glm::vec2>
{
  return _text_engine.parse_text(text, pos, size, color, _vertices, _indices);
}

void GraphicsEngine::text_mask_render()
{
  // update
  auto& buffer = _glyphs_buffers[_current_frame];
  buffer.clear();

  buffer.append_range(_vertices);
  auto offset = buffer.size();
  buffer.append_range(_indices);

  vkCmdBindIndexBuffer(get_current_frame().cmd, buffer.handle(), offset, VK_INDEX_TYPE_UINT16);

  // draw
  auto& cmd = get_current_frame().cmd;

  auto pc = PushConstant_text_mask
  {
    .buffer        = buffer.address(),
    .window_extent = _window->get_framebuffer_size(),
  };

  _text_mask_render_pipeline.bind(cmd, pc);

  vkCmdDrawIndexed(cmd, _indices.size(), 1, 0, 0, 0);

  _vertices.clear();
  _indices.clear();
}

}}