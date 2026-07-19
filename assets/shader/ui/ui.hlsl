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

static const float  msdf_px_range              = 4.0;
static const float  outline_reference_px_range = 2.0;
static const float2 px_range                   = float2(msdf_px_range, msdf_px_range);
static const float  outline_width_scale        = outline_reference_px_range / msdf_px_range;

float screen_px_range(float2 extent, float2 uv)
{
  float2 unit_range = px_range / extent;
  float2 screen_tex_size = 1.0 / fwidth(uv);
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

  // reference: https://www.redblobgames.com/x/2404-distance-field-effects/
  float outer_alpha = clamp(screen_range * (distance_from_edge + arg.outer_width * outline_width_scale) + 0.5, 0.0, 1.0);
  if (sd == 0.0)
    outer_alpha = 0.0;

  float outline_alpha    = max(outer_alpha - inner_alpha, 0.0);
  float inner_coverage   = inner_color.a * inner_alpha;
  float outline_coverage = outer_color.a * outline_alpha;
  float alpha            = saturate(inner_coverage + outline_coverage);

  if (alpha == 0.0)
    return float4(0.0, 0.0, 0.0, 0.0);

  float3 rgb = (inner_color.rgb * inner_coverage + outer_color.rgb * outline_coverage) / alpha;
  return float4(rgb, alpha);
}
