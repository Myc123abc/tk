#include "common.hlsl"

struct PS_Param
{
  float4 pos : SV_POSITION;
  float4 col : COLOR;
  float2 uv  : TEXCOORD;
  nointerpolation uint16_t type        : TYPE;
  nointerpolation uint16_t img_idx     : IMG_IDX;
  nointerpolation float4   outer_col   : OUTER_COL;
  nointerpolation float    outer_width : OUTER_WIDTH;
};

PS_Param vs(VS_Param arg)
{
  PS_Param res;
  res.pos         = float4((arg.pos + constants.window_pos + constants.composite_offset) / constants.render_target_extent * float2(2, -2) + float2(-1, 1), 0, 1);
  res.col         = color(arg.col);
  res.uv          = arg.uv;
  res.type        = arg.packed >> 16;
  res.img_idx     = arg.packed & 0xffff;
  res.outer_col   = color(arg.outer_col);
  res.outer_width = arg.outer_width;
  return res;
}

#define Vtx_Type_Shape 0
#define Vtx_Type_Image 1
#define Vtx_Type_Text  2

float2 px_range = float2(2, 2);

float screen_px_range(float2 extent, uint2 uv)
{
  float2 unit_range = px_range / extent;
  float2 screen_tex_size = rsqrt(sqrt(ddx(uv.x)) + sqrt(ddy(uv.y)));
  return max(0.5 * dot(unit_range, screen_tex_size), 1.0);
}

float median(float r, float g, float b) { return max(min(r, g), min(max(r, g), b)); }

float4 ps(PS_Param arg) : SV_TARGET
{
  if (arg.type == Vtx_Type_Shape)
    return arg.col;
  else if (arg.type == Vtx_Type_Image)
    return images[NonUniformResourceIndex(arg.img_idx)].Sample(g_sampler, arg.uv) * arg.col;

  //
  // text render
  //
  Texture2D img = images[NonUniformResourceIndex(arg.img_idx)];

  uint2 extent;
  img.GetDimensions(extent.x, extent.y);

  float3 msd = img.Sample(g_sampler, arg.uv).rgb;
  float sd   = median(msd.r, msd.g, msd.b);
  float screen_range       = screen_px_range(extent, arg.uv);
  float distance_from_edge = sd - 0.5;
  float inner_alpha        = clamp(screen_range * distance_from_edge + 0.5, 0.0, 1.0);

  float4 inner_color = arg.col;
  float4 outer_color = arg.outer_col;

  if (outer_color.a == 0.0)
    return float4(inner_color.rgb, inner_color.a * inner_alpha);

  if (inner_color.a == 0)
    inner_color = float4(0.0, 0.0, 0.0, 0.0);
  else
    inner_color *= inner_alpha;

  // reference: https://www.redblobgames.com/x/2404-distance-field-effects/
  float outer_alpha = clamp(screen_range * (distance_from_edge + arg.outer_width) + 0.5, 0.0, 1.0);
  return inner_color + (outer_color * (outer_alpha - inner_alpha));
}
