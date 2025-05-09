#version 460

#include "SMAA.h"

layout(location = 0) in vec2 tex_coord;
layout(location = 1) in vec4 offset[3];

layout(location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D image;

#define SMAA_INCLUDE_VS 0
#include "SMAA.hlsl"

void main()
{
	color = vec4(SMAAColorEdgeDetectionPS(tex_coord, offset, image), 0.0, 0.0);
	//color = vec4(SMAALumaEdgeDetectionPS(tex_coord, offset, image), 0.0, 0.0);
}