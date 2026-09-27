#include "common.hlsl"

struct PS_Param
{
  float4 pos : SV_POSITION;
  float4 col : COLOR;
};

PS_Param vs(VS_Param arg)
{
  PS_Param res;
  res.pos = float4((arg.pos + constants.mask_offset) / constants.render_target_extent * float2(2, -2) + float2(-1, 1), 0, 1);
  res.col = color(arg.col);
  return res;
}

float ps(PS_Param args) : SV_TARGET
{
  return args.col.a;
}
