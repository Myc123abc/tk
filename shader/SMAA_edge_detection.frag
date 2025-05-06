#version 460

#include "SMAA_edge_detection.h"

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 offset[3];
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

#define SMAA_INCLUDE_VS 0
#include "SMAA.hlsl"

void main() {
	if (params.SMAA_EDT == 0.0) {
		// Color (higher quality but slower)
		FragColor = vec4(SMAAColorEdgeDetectionPS(vTexCoord, offset, Source), 0.0, 0.0);
	} else {
		// Luma (faster but lower quality)
		FragColor = vec4(SMAALumaEdgeDetectionPS(vTexCoord, offset, Source), 0.0, 0.0);
	}
}