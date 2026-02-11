struct Constants
{
  uint32_t mipmap_count;
};

ConstantBuffer<Constants> constants : register(b0);
SamplerState              g_sampler : register(s0);
Texture2D                 image     : register(t0);
RWTexture2D<float4>       output    : register(u0);

groupshared float r[64];
groupshared float g[64];
groupshared float b[64];
groupshared float a[64];

float4 sample_color(bool is_width_even, bool is_height_even, float2 pos, uint32_t width, uint32_t height, uint32_t mip_level)
{
  float2 uv;
  float4 color;
  if (is_width_even && is_height_even)
  {
    uv    = (pos + 0.5) / float2(width, height);
    color = image.SampleLevel(g_sampler, uv, mip_level);
  }
  return color;
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
  uint mip_level = 0;

  // get image extent and mipmap counts
  uint width, height, mipmap_count;
  image.GetDimensions(mip_level, width, height, mipmap_count);
  ++mip_level;

  // width even height even
  bool is_width_even  = width  % 2 == 0;
  bool is_height_even = height % 2 == 0;

  output[id.xy] = sample_color(is_width_even, is_height_even, id.xy, width, height, mip_level);
}
