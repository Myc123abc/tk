#version 460

#include "text_render.h"

vec2 vertices[] =
{
  { 0, 0 },
  { 208 * 2, 0 },
  { 0, 208 * 2 },
};
vec2 uvs[] =
{
  { 0, 0 },
  { 2, 0 },
  { 0, 2 },
};

layout(location = 0) out vec2 uv;

void main()
{
  vec2 vertex = vertices[gl_VertexIndex];

  // set vertex position
  vec2 scale     = 2 / vec2(208);
  vec2 translate = vec2(-1, -1);
  gl_Position    = vec4((vertex + vec2(0)) * scale + translate, 0, 1);
  uv = uvs[gl_VertexIndex];
}