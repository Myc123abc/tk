struct Constants
{
  float2 texel_size;
};

ConstantBuffer<Constants> constants : register(b0);
SamplerState              g_sampler : register(s0);
Texture2D                 src       : register(t0);
RWTexture2D<float4>       dst       : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
  uint width, height, mip_level_count;
  src.GetDimensions(0, width, height, mip_level_count);

  uint2 pos = id.xy * 2;

  float4 color = 0;
  uint   count = 0;

  [unroll]
  for (uint dy = 0; dy < 2; ++dy)
  {
    [unroll]
    for (uint dx = 0; dx < 2; ++dx)
    {
      uint2 p = pos + uint2(dx, dy);
      if (p.x < width && p.y < height)
      {
        float2 uv = (float2(p) + 0.5) * constants.texel_size;
        color += src.SampleLevel(g_sampler, uv, 0);
        ++count;
      }
    }
  }

  dst[id.xy] = color / count;
}
