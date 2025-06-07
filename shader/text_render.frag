#version 460

#include "text_render.h"

layout(location = 0) in  vec2 uv;
layout(location = 0) out vec4 col;

layout(binding = 0) uniform sampler2D ft_image;

void main()
{
  col = pc.color * vec4(1, 1, 1, texture(ft_image, uv).r);
}