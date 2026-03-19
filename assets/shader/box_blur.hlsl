struct Constants
{
  uint2 extent;
  uint  radius;
  uint  horizontal;
};

ConstantBuffer<Constants> constants : register(b0);
Texture2D<float4>         src       : register(t0);
RWTexture2D<float4>       dst       : register(u0);

float4 load_clamped(int2 pos)
{
  int2 max_pos = int2(constants.extent) - 1;
  pos = clamp(pos, int2(0, 0), max_pos);
  return src.Load(int3(pos, 0));
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
  if (id.x >= constants.extent.x || id.y >= constants.extent.y)
    return;

  int2 pos = int2(id.xy);
  int2 dir = constants.horizontal ? int2(1, 0) : int2(0, 1);

  float inv_count = 1.0 / float(constants.radius * 2 + 1);
  float4 sum = 0.0;

  for (int i = -int(constants.radius); i <= int(constants.radius); ++i)
    sum += load_clamped(pos + dir * i);

  dst[pos] = sum * inv_count;
}
