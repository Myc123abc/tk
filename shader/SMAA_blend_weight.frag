#version 460

#include "SMAA.h"

layout(location = 0) in vec2 tex_coord;
layout(location = 1) in vec2 pix_coord;
layout(location = 2) in vec4 offset[3];

layout(location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D image;
layout(binding = 1) uniform sampler2D area_tex;
layout(binding = 2) uniform sampler2D search_tex;

#define SMAA_INCLUDE_VS 0
#include "SMAA.hlsl"

void main()
{
  // TODO: use for advance smaa
  vec4 subsample_indices = vec4(0.0);
	color = SMAABlendingWeightCalculationPS(tex_coord, pix_coord, offset, image, area_tex, search_tex, subsample_indices);
}