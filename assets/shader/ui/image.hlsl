#include "common.hlsl"

struct PS_Param
{
  float4 pos : SV_POSITION;
  float2 uv  : TEXCOORD;
};

PS_Param vs(VS_Param arg)
{
  PS_Param res;
  res.pos = float4((arg.pos.xy + constants.window_pos) / constants.render_target_extent * float2(2, -2) + float2(-1, 1), 0, 1);
  res.uv  = arg.uv;
  return res;
}

float4 ps(PS_Param args) : SV_TARGET
{
  float4 color = image.Sample(g_sampler, args.uv);
  return float4(color.rgb, color.a * constants.image_alpha);
}
