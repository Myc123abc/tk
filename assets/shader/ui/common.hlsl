// size limit 64 WORDs(32 bits), 256 bytes, 64 float, 16 float4
struct Constants
{
  uint2  render_target_extent;
  uint2  window_extent;

  float2 window_pos;
  float  shadow_thickness;
  float  shadow_radius;

  float3 shadow_color;
  float  shadow_softness;

  float4 wireframe_color;

  float2 mask_offset;
  float2 mask_extent;

  float2 composite_offset;
  float2 composite_extent;

  uint   draw_wireframe;
};

ConstantBuffer<Constants> constants       : register(b0);
SamplerState              g_sampler       : register(s0);
Texture2D                 images[]        : register(t0);
Texture2D                 mask_image      : register(t0, space1);
Texture2D                 composite_image : register(t0, space2);

struct VS_Param
{
  float2 pos         : POSITION;
  float2 uv          : TEXCOORD;
  uint   col         : COLOR;
  uint   packed      : PACKED;
  uint   outer_col   : OUTER_COL;
  float  outer_width : OUTER_WIDTH;
};

float4 color(uint c)
{
  float inv255 = 1.0 / 255.0;

  return float4
  (
    ((c >> 24) & 0xFF) * inv255,
    ((c >> 16) & 0xFF) * inv255,
    ((c >>  8) & 0xFF) * inv255,
    ((c      ) & 0xFF) * inv255
  );
}
