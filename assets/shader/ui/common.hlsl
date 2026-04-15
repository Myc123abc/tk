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

  uint   draw_wireframe;
  float  image_alpha;
};

ConstantBuffer<Constants> constants : register(b0);
SamplerState              g_sampler : register(s0);
Texture2D                 image     : register(t0);

struct VS_Param
{
  float2 pos : POSITION;
  float4 col : COLOR;
  float2 uv  : TEXCOORD;
};
