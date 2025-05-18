#version 460

#include "2D.h"

layout(location = 0) out struct
{
  vec4 col;
  vec2 uv;
} Out;

void main()
{
  // get vertex
  Vertex vertex  = pc.vertices.data[gl_VertexIndex];
  
  // set vertex position
  vec2 scale     = 2 / pc.window_extent;
  vec2 translate = vec2(-1, -1);
  gl_Position    = vec4((vertex.pos + pc.display_pos) * scale + translate, 0, 1);

  // get color
  float r = float((vertex.col >> 24) & 0xFF) / 255;
  float g = float((vertex.col >> 16) & 0xFF) / 255;
  float b = float((vertex.col >> 8 ) & 0xFF) / 255;
  float a = float((vertex.col      ) & 0xFF) / 255;
  Out.col = vec4(r, g, b, a);

  // set uv
  Out.uv  = vertex.uv;
}
