#version 460

#include "SMAA.h"

layout(location = 0) in vec2 tex_coord;
layout(location = 1) in vec4 offset;

layout(location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D image;
layout(binding = 4) uniform sampler2D blend_image;

#define SMAA_INCLUDE_VS 0
#include "SMAA.hlsl"

void main()
{
	color = SMAANeighborhoodBlendingPS(tex_coord, offset, image, blend_image);
  color = vec4(0,1,0,1);
}