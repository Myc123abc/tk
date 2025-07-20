#include "tk/GraphicsEngine/GraphicsEngine.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/GraphicsEngine/vk_extension.hpp"
#include "tk/GraphicsEngine/config.hpp"

#include <glm/gtc/matrix_transform.hpp>


namespace tk { namespace graphics_engine {

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

void GraphicsEngine::sdf_render(std::span<Vertex> vertices, std::span<uint16_t> indices, std::span<ShapeProperty> shape_properties)
{
  // get buffer and clear
  auto& buffer = _buffers[_current_frame].clear();

  // upload vertices to buffer
  buffer.append_range(vertices);

  // get offset of indices
  auto indices_offset = buffer.size();
  // upload indices to buffer
  buffer.append_range(indices);

  // convert shape properties to binary data
  uint32_t total_size{};
  for (auto const& property : shape_properties)
    total_size += ShapeProperty::header_field_count + property.values.size();
  std::vector<uint32_t> data;
  data.reserve(total_size);
  for (auto const& property : shape_properties)
  {
    data.emplace_back(std::bit_cast<uint32_t>(property.type));
    data.emplace_back(std::bit_cast<uint32_t>(property.color.r));
    data.emplace_back(std::bit_cast<uint32_t>(property.color.g));
    data.emplace_back(std::bit_cast<uint32_t>(property.color.b));
    data.emplace_back(std::bit_cast<uint32_t>(property.color.a));
    data.emplace_back(std::bit_cast<uint32_t>(property.thickness));
    data.emplace_back(std::bit_cast<uint32_t>(property.op));
    for (auto value : property.values)
      data.emplace_back(std::bit_cast<uint32_t>(value));
  }
  // get offset of shape properties
  auto shape_properties_offset = buffer.size();
  // upload shape properties to buffer
  buffer.append_range(data);
  
  auto& cmd = get_current_frame().cmd;

  // bind index buffer
  vkCmdBindIndexBuffer(cmd, buffer.handle(), indices_offset, VK_INDEX_TYPE_UINT16);

  auto pc = PushConstant_SDF
  {
    .vertices         = buffer.address(),
    .shape_properties = buffer.address() + shape_properties_offset,
    .window_extent    = _window->get_framebuffer_size(),
  };

  _sdf_render_pipeline.bind(cmd, pc);

  vkCmdDrawIndexed(cmd, indices.size(), 1, 0, 0, 0);
}

auto GraphicsEngine::parse_text(std::string_view text, glm::vec2 const& pos, float size, bool italic, std::vector<Vertex>& vertices, std::vector<uint16_t>& indices, uint32_t offset, uint16_t& idx) -> std::pair<glm::vec2, glm::vec2>
{
  return _text_engine.parse_text(text, pos, size, italic, vertices, indices, offset, idx);
}

void GraphicsEngine::upload_glyph(std::span<msdfgen::Bitmap<float, 4>> bitmaps)
{
  if (bitmaps.empty()) return;

  _font_atlas_buffer.clear();
  uint32_t   buffer_offset{};
  uint32_t   idx{};
  VkOffset2D image_offset{ static_cast<int32_t>(_glyph_uvs[idx].x), static_cast<int32_t>(_glyph_uvs[idx].y) };
  auto cmd = _command_pool.create_command().begin();

  // TODO: optimization, use tmp cpu buffer of atlas, copy bitmap to tmp buffer with padding, and copy tmp buffer data to atlas image,
  //       also if tmp buffer data is not a rectangle, split it to mulitple rectangle then upload mulitple times.
  for (msdfgen::BitmapConstRef<float, 4> bitmap : bitmaps)
  {
    // copy bitmap to buffer
    auto byte_size = bitmap.height * bitmap.width * 4 * 4;
    _font_atlas_buffer.append(bitmap.pixels, byte_size);

    // get image offset
    image_offset = { static_cast<int32_t>(_glyph_uvs[idx].x), static_cast<int32_t>(_glyph_uvs[idx].y) };
    ++idx;

    // upload data of buffer to atlas
    copy(cmd, _font_atlas_buffer, buffer_offset, _font_atlas_image, image_offset, { (uint32_t)bitmap.width, (uint32_t)bitmap.height }); 

    // set next buffer offset
    buffer_offset += byte_size;
  }

  _font_atlas_image.set_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  cmd.end().submit_wait_free(_command_pool, _graphics_queue);

  _glyph_uvs.clear();
}

// TODO:
// get next glyph position by extent
// if current atlas is full, create new atlas image and reset glyph position
auto GraphicsEngine::get_glyph_pos(glm::vec2 const& extent) noexcept -> glm::vec2
{
repeat:
  // get next position
  auto max = _glyph_pos + extent;

  // directly can store
  if (max.x <= Font_Atlas_Width && max.y <= Font_Atlas_Height)
  {
    _glyph_uvs.emplace_back(_glyph_pos);
    auto origin = _glyph_pos;
    _glyph_pos.x = max.x;
    // update max height of glyph on current line
    _max_glyph_height = std::max(_max_glyph_height, extent.y);
    return origin;
  }
  else if (max.x > Font_Atlas_Width)
  {
    // move to next line
    _glyph_pos.x = 0;
    _glyph_pos.y += _max_glyph_height;
    _max_glyph_height = 0;
    goto repeat;
  }
  else if (max.y > Font_Atlas_Height)
  {
    throw_if(true, "TODO: signel font altas image is full, create new one");
    goto repeat;
  }
  assert(true);
  return {};
}

}}