#include "tk/GraphicsEngine/GraphicsEngine.hpp"
#include "tk/GraphicsEngine/vk_extension.hpp"
#include "tk/GraphicsEngine/config.hpp"
#include "tk/util.hpp"

#include <glm/gtc/matrix_transform.hpp>


namespace tk { namespace graphics_engine {

auto GraphicsEngine::frame_begin() -> bool
{
  if (_frames.acquire_swapchain_image(_wait_fence) == false)
    return false;

  auto& cmd = _frames.get_command();

  set_pipeline_state(cmd);

  if (config()->use_descriptor_buffer)
    bind_descriptor_buffer(cmd, _descriptor_buffer);

  return true;
}

void GraphicsEngine::frame_end()
{
  _frames.present_swapchain_image(_graphics_queue, _present_queue);
}

void GraphicsEngine::render_begin(Image& image)
{
  auto& cmd = _frames.get_command();

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
  render_begin(_frames.get_swapchain_image());
}

void GraphicsEngine::set_pipeline_state(Command const& cmd)
{
  auto& swapchain_image = _frames.get_swapchain_image();
  VkViewport viewport
  {
    .width  = static_cast<float>(swapchain_image.extent3D().width),
    .height = static_cast<float>(swapchain_image.extent3D().height), 
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
    VkRect2D scissor{ .extent = swapchain_image.extent2D(), };
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
    VkRect2D scissor{ .extent = swapchain_image.extent2D(), };
    vkCmdSetScissor(cmd, 0, 1, &scissor);
  }
}

void GraphicsEngine::render_end()
{
  vkCmdEndRendering(_frames.get_command());
}

void GraphicsEngine::sdf_render(std::span<Vertex> vertices, std::span<uint16_t> indices, std::span<ShapeProperty> shape_properties)
{
  // upload vertices to buffer
  _frames.append_range(vertices);

  // get offset of indices
  auto indices_offset = _frames.size();
  // upload indices to buffer
  _frames.append_range(indices);

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
  auto shape_properties_offset = util::align_size(_frames.size(), 8);
  // add padding for 8 bytes alignment
  _frames.append_range(std::vector<std::byte>(shape_properties_offset - _frames.size()));
  // upload shape properties to buffer
  _frames.append_range(data);
  
  auto& cmd = _frames.get_command();

  _frames.upload();
  auto [handle, address] = _frames.get_handle_and_address();

  // bind index buffer
  vkCmdBindIndexBuffer(cmd, handle, _frames.get_current_frame_byte_offset() + indices_offset, VK_INDEX_TYPE_UINT16);

  auto pc = PushConstant_SDF
  {
    .vertices         = address,
    .shape_properties = address + shape_properties_offset,
    .window_extent    = _window->get_framebuffer_size(),
  };

  _sdf_render_pipeline.bind(cmd, pc);

  vkCmdDrawIndexed(cmd, indices.size(), 1, 0, 0, 0);
}

auto GraphicsEngine::parse_text(std::string_view text, glm::vec2 pos, float size, type::FontStyle style, std::vector<Vertex>& vertices, std::vector<uint16_t>& indices, uint32_t offset, uint16_t& idx) -> glm::vec2
{
  auto [text_pos_info, u32str] = _text_engine.calculate_text_pos_info(text, style);

  // get some glyphs not cached
  if (_text_engine.has_uncached_glyphs(u32str, style))
  {
    // upload them
    auto cmd = _command_pool.create_command().begin();
    _text_engine.generate_sdf_bitmaps(cmd);
    cmd.end().submit_wait_free(_command_pool, _graphics_queue); // TODO: use single cmd every frame after start rendering
                                                                //       use Semaphores to replace wait
  }

  // add vertices and indices
  vertices.reserve(vertices.size() + u32str.size() * 4);
  indices.reserve(indices.size() + u32str.size() * 6);
  for (auto i = 0; i < u32str.size(); ++i)
  {
    auto glyph_info = _text_engine.get_cached_glyph_info(u32str[i], style);
    // TODO: change 0 to atlases index
    vertices.append_range(glyph_info->get_vertices(pos, size, offset, text_pos_info.max_ascender, 0)); // TODO: vertices and indices generate performance worse
    indices.append_range(GlyphInfo::get_indices(idx));
    pos = GlyphInfo::get_next_position(pos, size, text_pos_info.advances[i]);
  }
  return { vertices.back().pos.x, text_pos_info.max_height };
}

}}