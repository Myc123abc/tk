#include "common.hlsl"

struct PS_Param
{
  float4 pos : SV_POSITION;
  float4 col : COLOR;
  float2 uv  : TEXCOORD;
};

PS_Param vs(VS_Param arg)
{
  PS_Param res;
  res.pos = float4((arg.pos + constants.composite_offset) / constants.render_target_extent * float2(2, -2) + float2(-1, 1), 0, 1);
  res.col = arg.col;
  res.uv  = arg.uv;
  return res;
}

float4 ps(PS_Param args) : SV_TARGET
{
  return image.Sample(g_sampler, args.uv) * args.col;
}
