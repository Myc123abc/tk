#version 460

#include "SDF.h"

layout(location = 0) out vec2 uv;
layout(location = 1) flat out uint offset;
layout(location = 2) flat out uint glyph_atlases_index;

void main()
{
  Vertex vertex = pc.vertices.data[gl_VertexIndex];

  gl_Position = vec4(vertex.pos / pc.window_extent * vec2(2) - vec2(1), 0, 1);

  uv                  = vertex.uv;
  offset              = vertex.offset;
  glyph_atlases_index = vertex.glyph_atlases_index;
}