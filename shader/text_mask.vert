//
// This render pass use to render text on a independent image.
// We need to render text first then render sdf shapes.
// Rendering sdf shapes use quads of glyphs and this independent image.
// To sample the the result of text and integrate with other sdf shapes for right depth relationship.
//
// TODO: Currently, only single font. We need expand to multi-font-atlas processing in future.
//

#version 460

#include "text_mask.h"

layout(location = 0) out vec2 uv;

void main()
{
  vec2 min = pc.pos.xy;
  vec2 max = pc.pos.zw;

  min = min / pc.window_extent * vec2(2) - vec2(1);
  max = max / pc.window_extent * vec2(2) - vec2(1);

  vec2 vertices[] =
  {
    min,
    { max.x, min.y },
    { min.x, max.y },
    max,
  };
  gl_Position = vec4(vertices[gl_VertexIndex], 0, 1);

  vec2 uvs[] =
  {
    { pc.uv.x, pc.uv.w },
    { pc.uv.z, pc.uv.w },
    { pc.uv.x, pc.uv.y },
    { pc.uv.z, pc.uv.y },
  };
  uv = uvs[gl_VertexIndex];
}