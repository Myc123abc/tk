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

  float4 outer_color;

  float  outline_width;
  uint   draw_wireframe;
};

ConstantBuffer<Constants> constants  : register(b0);
SamplerState              g_sampler  : register(s0);
Texture2D                 image      : register(t0);
Texture2D                 mask_image : register(t0, space1);
Texture2D                 images[]   : register(t0, space2);

struct VS_Param
{
  float2 pos       : POSITION;
  float2 uv        : TEXCOORD;
  uint   col       : COLOR;
  uint   image_idx : IMAGE_IDX;
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
