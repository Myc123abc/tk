#include "tk/GraphicsEngine/GraphicsEngine.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/GraphicsEngine/vk_extension.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace tk { namespace graphics_engine {

auto get_cursor_position() -> glm::vec2;

auto GraphicsEngine::frame_begin() -> bool
{
  auto* frame = &get_current_frame();

  throw_if(vkWaitForFences(_device, 1, &frame->fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS,
           "failed to wait fence");
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

void GraphicsEngine::render_begin()
{
  auto& frame = get_current_frame();

  get_current_swapchain_image().set_layout(frame.cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  
  auto color_attachment = VkRenderingAttachmentInfo
  {
    .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .imageView          = get_current_swapchain_image().view(),
    .imageLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .loadOp             = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .storeOp            = VK_ATTACHMENT_STORE_OP_STORE,
  };
  auto rendering = VkRenderingInfo
  {
    .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
    .renderArea           = { .extent = get_current_swapchain_image().extent2D(), },
    .layerCount           = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments    = &color_attachment,
  };
  vkCmdBeginRendering(frame.cmd, &rendering);

  auto stages  = std::vector<VkShaderStageFlagBits>{ VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };
  auto shaders = std::vector<VkShaderEXT>{ _sdf_vert, _sdf_frag };
  graphics_engine::vkCmdBindShadersEXT(frame.cmd, stages.size(), stages.data(), shaders.data());
}

void GraphicsEngine::set_pipeline_state(Command const& cmd)
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
  VkViewport viewport
  {
    .width  = static_cast<float>(get_current_swapchain_image().extent3D().width),
    .height = static_cast<float>(get_current_swapchain_image().extent3D().height), 
    .maxDepth = 1.f,
  };
  VkRect2D scissor{ .extent = get_current_swapchain_image().extent2D(), };
  vkCmdSetViewportWithCount(cmd, 1, &viewport);
  vkCmdSetScissorWithCount(cmd, 1, &scissor);
  // TODO: need to change to use triangle list with indices
  vkCmdSetPrimitiveTopology(cmd, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
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

void GraphicsEngine::render_end()
{
  auto& frame = get_current_frame();
  vkCmdEndRendering(frame.cmd);
}

void GraphicsEngine::update(std::span<glm::vec2> points, std::span<ShapeInfo> infos)
{
  _buffers[_current_frame].clear().append(points).append(infos);
}

void GraphicsEngine::render(uint32_t offset, uint32_t num)
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

  vkCmdPushConstants(cmd, _sdf_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);

  vkCmdDraw(cmd, 4, 1, 0, 0);
}

void GraphicsEngine::text_render_begin()
{
  auto& frame = get_current_frame();

  _text_rendering_image.set_layout(frame.cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  
  auto color_attachment = VkRenderingAttachmentInfo
  {
    .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .imageView          = _text_rendering_image.view(),
    .imageLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .loadOp             = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .storeOp            = VK_ATTACHMENT_STORE_OP_STORE,
  };
  auto rendering = VkRenderingInfo
  {
    .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
    .renderArea           = { .extent = _text_rendering_image.extent2D(), },
    .layerCount           = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments    = &color_attachment,
  };
  vkCmdBeginRendering(frame.cmd, &rendering);

  auto stages  = std::vector<VkShaderStageFlagBits>{ VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };
  auto shaders = std::vector<VkShaderEXT>{ _text_rendering_vert, _text_rendering_frag };
  graphics_engine::vkCmdBindShadersEXT(frame.cmd, stages.size(), stages.data(), shaders.data());
}

void GraphicsEngine::text_render()
{
  auto& frame = get_current_frame();

  // TODO: tmp, test, render text
  auto metrics = _font_geo.getMetrics();
  //auto scale = 1.0 / (metrics.ascenderY - metrics.descenderY);
  auto glyph   = _font_geo.getGlyph('g');
  
  double al, ab, ar, at;
  glyph->getQuadAtlasBounds(al, ab, ar, at);
  double pl, pb, pr, pt;
  glyph->getQuadPlaneBounds(pl, pb, pr, pt);
  auto glyph_pos = glm::vec4(pl, -pb, pr, -pt);
  
  auto window_extent = _window->get_framebuffer_size();

  auto pos  = glm::vec2(0, 60);
  auto size = 32.f; // TODO: use emSize to scale glyph size

  auto distance = pos - window_extent / glm::vec2(2);
  auto move = distance / window_extent * glm::vec2(2);

  auto scale = _font_atlas_extent / window_extent;
  
  glyph_pos.x *= scale.x;
  glyph_pos.y *= scale.y;
  glyph_pos.z *= scale.x;
  glyph_pos.w *= scale.y;
  glyph_pos.x += move.x;
  glyph_pos.y += move.y;
  glyph_pos.z += move.x;
  glyph_pos.w += move.y;

  bind_descriptor_buffer(frame.cmd, _descriptor_buffer.address(), VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT, _text_rendering_pipeline_layout, VK_PIPELINE_BIND_POINT_GRAPHICS);
  auto pc = PushConstant_text_rendering
  {
    .pos = glyph_pos,
    .uv   = { al / _font_atlas_extent.x, ab / _font_atlas_extent.y, ar / _font_atlas_extent.x, at / _font_atlas_extent.y },
  };
  vkCmdPushConstants(frame.cmd, _text_rendering_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);
  vkCmdDraw(frame.cmd, 4, 1, 0, 0);
}

void GraphicsEngine::text_render_end()
{
  auto& frame = get_current_frame();
  vkCmdEndRendering(frame.cmd);
}


} }
