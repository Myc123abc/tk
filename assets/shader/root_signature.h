struct Constants
{
  uint2  window_extent;
  float2 window_pos;
  float  image_alpha;
  bool   is_image; // FIXME: discard
};

ConstantBuffer<Constants> constants : register(b0);
SamplerState              g_sampler : register(s0);
ByteAddressBuffer         buffer    : register(t0);
Texture2D                 image     : register(t0, space1);
Texture2D                 images[]  : register(t0, space2);