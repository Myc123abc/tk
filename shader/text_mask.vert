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
  Vertex vertex  = pc.vertices.data[gl_VertexIndex];
  gl_Position = vec4(vertex.pos / pc.window_extent * vec2(2) - vec2(1), 0, 1);
  uv = vertex.uv;
}