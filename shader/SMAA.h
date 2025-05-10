layout(push_constant) uniform PushConstant
{
	vec4 smaa_rt_metrics;
} pc;

#define SMAA_RT_METRICS pc.smaa_rt_metrics
#define SMAA_GLSL_4
#define SMAA_PRESET_ULTRA

#define SMAA_INCLUDE_PS 0
#define SMAA_INCLUDE_VS 0
#include "SMAA.hlsl"

layout(local_size_x = 16, local_size_y = 16) in;