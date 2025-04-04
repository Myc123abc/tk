#include "GraphicsEngine.hpp"
#include "PipelineBuilder.hpp"
#include "init-util.hpp"

namespace tk { namespace graphics_engine {

void GraphicsEngine::create_graphics_pipeline()
{
  Shader vertex_shader(_device, "build/triangle_vert.spv");
  Shader fragment_shader(_device, "build/triangle_frag.spv");

  VkPipelineLayoutCreateInfo layout_info
  {
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
  };
  throw_if(vkCreatePipelineLayout(_device, &layout_info, nullptr, &_graphics_pipeline_layout) != VK_SUCCESS,
           "failed to create graphics pipeline layout");

  _graphics_pipeline = PipelineBuilder()
                      .set_shaders(vertex_shader.shader, fragment_shader.shader)
                      .set_cull_mode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE)
                      .set_color_attachment_format(_image.format)
                      .build(_device, _graphics_pipeline_layout);

  _destructors.push([this]
  { 
    vkDestroyPipeline(_device, _graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(_device, _graphics_pipeline_layout, nullptr);
  });
}

} }
