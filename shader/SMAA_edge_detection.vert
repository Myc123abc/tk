#version 460

#include "SMAA_edge_detection.h"

layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 offset[3];

#define SMAA_INCLUDE_PS 0
#include "SMAA.hlsl"

void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord;
	SMAAEdgeDetectionVS(TexCoord, offset);
}
