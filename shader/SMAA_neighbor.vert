#version 460

#include "SMAA.h"

layout(location = 0) out vec2 tex_coord;
layout(location = 1) out vec4 offset;

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
	// set vertex position
  vec2 scale     = 2 / pc.smaa_rt_metrics.zw;
  vec2 translate = vec2(-1, -1);
  gl_Position    = vec4((vertices[gl_VertexIndex] * scale + translate), 0, 1);

	tex_coord = tex_coords[gl_VertexIndex];

	SMAANeighborhoodBlendingVS(tex_coord, offset);
}
