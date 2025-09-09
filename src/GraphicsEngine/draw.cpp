#include "GraphicsEngine.hpp"
#include "../util.hpp"

#include <glm/gtc/matrix_transform.hpp>


namespace tk { namespace graphics_engine {

auto GraphicsEngine::frame_begin() -> bool
{
  // acquire new frame's swapchain image
  if (_frames.acquire_swapchain_image(_wait_fence) == false)
    return false;

  // get current frame's command
  auto& cmd = _frames.get_command();

  // destroy old resources
  _frames.destroy_old_resources();

  // set resources for a new frame
  _sdf_buffer.frame_begin();
  if (_text_engine.frame_begin(cmd))
  {
    // TODO: can optimal use bigger descriptor pool and layout, then only update new descriptors?
    //       only recreate descriptor pool until pool is unenough
    // destroy old graphics pipeline
    _frames.push_old_resource([pipeline = this->_sdf_graphics_pipeline] { pipeline.destroy_without_shader_modules(); });  
    // create new one
    _sdf_graphics_pipeline.recreate(
    {
      { ShaderType::fragment, 0, _text_engine.get_glyph_atlases() },
    });
  }

  return true;
}

void GraphicsEngine::frame_end()
{
  //_frames.copy_image_to_swapchain(_offscreen_image);
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
  //render_begin(_offscreen_image);
  render_begin(_frames.get_swapchain_image());
}

void GraphicsEngine::render_end()
{
  vkCmdEndRendering(_frames.get_command());
}

void GraphicsEngine::sdf_render(std::span<Vertex> vertices, std::span<uint16_t> indices, std::span<ShapeProperty> shape_properties)
{
  // upload vertices to buffer
  _sdf_buffer.append_range(vertices);

  // get offset of indices
  auto indices_offset = _sdf_buffer.size();
  // upload indices to buffer
  _sdf_buffer.append_range(indices);

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
  auto shape_properties_offset = util::align_size(_sdf_buffer.size(), 8);
  // add padding for 8 bytes alignment
  _sdf_buffer.append_range(std::vector<std::byte>(shape_properties_offset - _sdf_buffer.size()));
  // upload shape properties to buffer
  _sdf_buffer.append_range(data);
  
  auto& cmd = _frames.get_command();

  _sdf_buffer.upload();

  auto [handle, address] = _sdf_buffer.get_handle_and_address();

  // bind index buffer
  vkCmdBindIndexBuffer(cmd, handle, _sdf_buffer.get_current_frame_byte_offset() + indices_offset, VK_INDEX_TYPE_UINT16);

  auto pc = PushConstant_SDF
  {
    .vertices         = address,
    .shape_properties = address + shape_properties_offset,
    .window_extent    = _window->get_framebuffer_size(),
  };

  // bind and set pipeline
  _sdf_graphics_pipeline.bind(cmd, pc);
  _sdf_graphics_pipeline.set_pipeline_state(cmd, _swapchain.extent());

  vkCmdDrawIndexed(cmd, indices.size(), 1, 0, 0, 0);
}

auto GraphicsEngine::parse_text(std::string_view text, glm::vec2 pos, float size, type::FontStyle style, std::vector<Vertex>& vertices, std::vector<uint16_t>& indices, uint32_t offset, uint16_t& idx) -> glm::vec2
{
  auto [text_pos_info, u32str] = _text_engine.calculate_text_pos_info(text, style);

  // get some glyphs not cached
  if (_text_engine.has_uncached_glyphs(u32str, style))
    _text_engine.generate_sdf_bitmaps();

  // add vertices and indices
  vertices.reserve(vertices.size() + u32str.size() * 4);
  indices.reserve(indices.size() + u32str.size() * 6);
  for (auto i = 0; i < u32str.size(); ++i)
  {
    auto glyph_info = _text_engine.get_cached_glyph_info(u32str[i], style);
    vertices.append_range(glyph_info->get_vertices(pos, size, offset, text_pos_info.max_ascender, glyph_info->glyph_atlas_index)); // TODO: vertices and indices generate performance worse
    indices.append_range(GlyphInfo::get_indices(idx));
    pos = GlyphInfo::get_next_position(pos, size, text_pos_info.advances[i]);
  }
  return { vertices.back().pos.x, text_pos_info.max_height * GlyphInfo::get_scale(size) };
}

}}