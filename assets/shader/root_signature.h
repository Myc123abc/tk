struct Constants
{
  uint2  render_target_extent;
  uint2  window_extent;
  float2 window_pos;
  float  shadow_thickness;
  float  shadow_radius;
  float3 shadow_color;
  float  shadow_softness;
  float  image_alpha;
};

ConstantBuffer<Constants> constants : register(b0);
SamplerState              g_sampler : register(s0);
ByteAddressBuffer         buffer    : register(t0);
Texture2D                 image     : register(t0, space1);
Texture2D                 images[]  : register(t0, space2);

struct Vertex
{
  float3   pos           : POSITION;
  float2   uv            : TEXCOORD;
  uint32_t buffer_offset : BUFFER_OFFSET;
};
