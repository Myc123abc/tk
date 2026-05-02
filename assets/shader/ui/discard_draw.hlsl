#include "common.hlsl"

struct PS_Param
{
  float4 pos : SV_POSITION;
  float2 uv  : TEXCOORD;
};

PS_Param vs(VS_Param arg)
{
  PS_Param res;
  res.pos = float4((arg.pos + constants.window_pos) / constants.render_target_extent * float2(2, -2) + float2(-1, 1), 0, 1);
  res.uv  = arg.uv;
  return res;
}

float4 ps(PS_Param args) : SV_TARGET
{
  float2 uv   = args.pos.xy / constants.render_target_extent;
  float4 col  = image.Sample(g_sampler, uv);
  float  mask = 1.0 - mask_image.Sample(g_sampler, uv).r;

  col.a *= mask;

  return col;
}
