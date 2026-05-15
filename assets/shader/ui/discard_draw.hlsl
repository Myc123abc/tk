#include "common.hlsl"

struct PS_Param
{
  float4 pos          : SV_POSITION;
  float2 mask_uv      : TEXCOORD0;
  float2 composite_uv : TEXCOORD1;
};

PS_Param vs(VS_Param arg)
{
  PS_Param res;
  res.pos          = float4((arg.pos + constants.window_pos) / constants.render_target_extent * float2(2, -2) + float2(-1, 1), 0, 1);
  res.mask_uv      = (arg.pos + constants.mask_offset) / constants.mask_extent;
  res.composite_uv = (arg.pos + constants.composite_offset) / constants.composite_extent;
  return res;
}

float4 ps(PS_Param args) : SV_TARGET
{
  float4 col = image.Sample(g_sampler, args.composite_uv);
  float mask = 1.0;
  if (args.mask_uv.x > 0 && args.mask_uv.x < 1 && args.mask_uv.y > 0 && args.mask_uv.y < 1)
    mask -= mask_image.Sample(g_sampler, args.mask_uv).r;

  if (col.a > 0.0) col.rgb /= col.a;
  col.a *= mask;

  return col;
}
