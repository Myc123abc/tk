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
  float  image_alpha; // FIXME: discard
};

ConstantBuffer<Constants> constants : register(b0);

struct VS_Param
{
  float2 pos : POSITION;
  float4 col : COLOR;
};

struct PS_Param
{
  float4 pos : SV_POSITION;
  float4 col : COLOR;
};

PS_Param vs(VS_Param arg)
{
  PS_Param res;
  res.pos = float4((arg.pos + constants.window_pos) / constants.render_target_extent * float2(2, -2) + float2(-1, 1), 1, 1);
  res.col = arg.col;
  return res;
}

float4 ps(PS_Param arg) : SV_TARGET
{
  return arg.col;
}
