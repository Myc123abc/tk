#version 460

#include "SMAA.h"

layout(location = 0) out vec2 tex_coord;
layout(location = 1) out vec2 pix_coord;
layout(location = 2) out vec4 offset[3];

#define SMAA_INCLUDE_PS 0
#include "SMAA.hlsl"

vec2 vertices[] =
{
	{ -1, -1 },
	{  3, -1 },
	{ -1,  3 },
};

vec2 tex_coords[] = 
{
	{ 0, 0 },
	{ 2, 0 },
	{ 0, 2 },
};

void main()
{
	gl_Position = vec4(vertices[gl_VertexIndex], 0, 1);
	
	tex_coord = tex_coords[gl_VertexIndex];

	SMAABlendingWeightCalculationVS(tex_coord, pix_coord, offset);
}
