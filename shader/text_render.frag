#version 460

#include "text_render.h"

layout(location = 0) in  vec2 uv;
layout(location = 0) out vec4 col;

layout(binding = 0) uniform sampler2D atlas;

void main()
{
  col = texture(atlas, vec2(uv.x, 1.0 - uv.y));
}