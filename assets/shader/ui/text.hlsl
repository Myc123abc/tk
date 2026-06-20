#include "common.hlsl"

struct PS_Param
{
  float4 pos : SV_POSITION;
  float4 col : COLOR;
  float2 uv  : TEXCOORD;
  nointerpolation uint image_idx : IMAGE_IDX;
};

PS_Param vs(VS_Param arg)
{
  PS_Param res;
  res.pos       = float4((arg.pos + constants.window_pos) / constants.render_target_extent * float2(2, -2) + float2(-1, 1), 0, 1);
  res.col       = arg.col;
  res.uv        = arg.uv;
  res.image_idx = arg.image_idx;
  return res;
}

float4 ps(PS_Param args) : SV_TARGET
{
  // reference: https://computergraphics.stackexchange.com/questions/306/sharp-corners-with-signed-distance-fields-fonts
  // author: Detheroc
  float d = images[NonUniformResourceIndex(args.image_idx)].Sample(g_sampler, args.uv).r - 0.5;
  float w = fwidth(d);
  float inner_alpha = clamp(d / w + 0.5, 0.0, 1.0);

  float4 color;
  float4 inner_color = args.col;
  float4 outer_color = constants.outer_color;

  if (outer_color.a == 0.0)
    color = float4(inner_color.rgb, inner_color.a * inner_alpha);
  else
  {
    if (inner_color.a == 0)
      inner_color = float4(0.0, 0.0, 0.0, 0.0);
    else
      inner_color *= inner_alpha;

    // reference: https://www.redblobgames.com/x/2404-distance-field-effects/
    float outer_alpha = clamp((d + constants.outline_width) / w + 0.5, 0.0, 1.0);
    color = inner_color + (outer_color * (outer_alpha - inner_alpha));
  }
  return color;
}
