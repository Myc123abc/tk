#version 460

#include "SDF.h"

layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 color;

vec4 to_vec4(uint x)
{
  float r = float((x >> 24) & 0xFF) / 255;
  float g = float((x >> 16) & 0xFF) / 255;
  float b = float((x >> 8 ) & 0xFF) / 255;
  float a = float((x      ) & 0xFF) / 255;
  return vec4(r, g, b, a);
}

void main()
{
  Vertex vertex = pc.vertices.data[gl_VertexIndex];

  gl_Position = vec4(vertex.pos / pc.window_extent * vec2(2) - vec2(1), 0, 1);

  uv    = vertex.uv;
  color = to_vec4(vertex.color);
}