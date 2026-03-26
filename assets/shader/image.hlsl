#include "root_signature.h"

struct PSParameter
{
  float4 pos : SV_POSITION;
  float2 uv  : TEXCOORD;
};

PSParameter vs(Vertex vertex)
{
  PSParameter res;
  res.pos = float4((vertex.pos.xy + constants.window_pos) / constants.render_target_extent * float2(2, -2) + float2(-1, 1), vertex.pos.z, 1);
  res.uv  = vertex.uv;
  return res;
}

float4 ps(PSParameter args) : SV_TARGET
{
  float4 color = image.Sample(g_sampler, args.uv);
  return float4(color.rgb, color.a * constants.image_alpha);
}