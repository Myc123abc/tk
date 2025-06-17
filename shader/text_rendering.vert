//
// This render pass use to render text on a independent image.
// We need to render text first then render sdf shapes.
// Rendering sdf shapes use quads of glyphs and this independent image.
// To sample the the result of text and integrate with other sdf shapes for right depth relationship.
//
// TODO: Currently, only single font. We need expand to multi-font-atlas processing in future.
//

// TODO: should i convert to uv image? because directly text image, in sdf also need sample

#version 460

#include "text_rendering.h"

layout(location = 0) out vec2 uv;

void main()
{
  vec2 vertices[] =
  {
    { pc.pos.x, pc.pos.w },
    { pc.pos.z, pc.pos.w },
    { pc.pos.x, pc.pos.y },
    { pc.pos.z, pc.pos.y },
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