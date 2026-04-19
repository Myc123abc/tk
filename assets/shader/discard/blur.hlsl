static const uint g_max_blur_radius = 5;

struct Constants
{
  uint   blur_radius;
  float3 padding;
  float4 weights[3];
};

ConstantBuffer<Constants> constants : register(b0);
Texture2D                 src       : register(t0);
RWTexture2D<float4>       dst       : register(u0);

#define N 128
#define CacheSize (N + 2 * g_max_blur_radius)
groupshared float4 g_cache[CacheSize];

[numthreads(N, 1, 1)]
void horizontal_pass(uint3 group_tid    : SV_GroupThreadID,
                     uint3 dispatch_tid : SV_DispatchThreadID)
{
  uint2 extent;
  src.GetDimensions(extent.x, extent.y);

  if (group_tid.x < constants.blur_radius)
  {
    int x = max((int)dispatch_tid.x - constants.blur_radius, 0);
    g_cache[group_tid.x] = src[uint2(x, dispatch_tid.y)];
  }
  if (group_tid.x >= N - constants.blur_radius)
  {
    int x = min((int)dispatch_tid.x + constants.blur_radius, extent.x - 1);
    g_cache[group_tid.x + 2 * constants.blur_radius] = src[uint2(x, dispatch_tid.y)];
  }

  g_cache[group_tid.x + constants.blur_radius] = src[min(dispatch_tid.xy, extent - 1)];

  GroupMemoryBarrierWithGroupSync();

  float4 color = 0;
  [unroll]
  for (int i = -g_max_blur_radius; i <= (int)g_max_blur_radius; ++i)
  {
    if (abs(i) <= constants.blur_radius)
    {
      int k = group_tid.x + constants.blur_radius + i;
      int v = i + constants.blur_radius;
      color += constants.weights[v / 4][v & 0x3] * g_cache[k];
    }
  }
  dst[dispatch_tid.xy] = color;
}

[numthreads(1, N, 1)]
void vertical_pass(uint3 group_tid    : SV_GroupThreadID,
                   uint3 dispatch_tid : SV_DispatchThreadID)
{
  uint2 extent;
  src.GetDimensions(extent.x, extent.y);

  if (group_tid.y < constants.blur_radius)
  {
    int y = max((int)dispatch_tid.y - constants.blur_radius, 0);
    g_cache[group_tid.y] = src[uint2(dispatch_tid.x, y)];
  }
  if (group_tid.y >= N - constants.blur_radius)
  {
    int y = min((int)dispatch_tid.y + constants.blur_radius, extent.y - 1);
    g_cache[group_tid.y + 2 * constants.blur_radius] = src[uint2(dispatch_tid.x, y)];
  }

  g_cache[group_tid.y + constants.blur_radius] = src[min(dispatch_tid.xy, extent - 1)];

  GroupMemoryBarrierWithGroupSync();

  float4 color = 0;
  [unroll]
  for (int i = -g_max_blur_radius; i <= (int)g_max_blur_radius; ++i)
  {
    if (abs(i) <= constants.blur_radius)
    {
      int k = group_tid.y + constants.blur_radius + i;
      int v = i + constants.blur_radius;
      color += constants.weights[v / 4][v & 0x3] * g_cache[k];
    }
  }
  dst[dispatch_tid.xy] = color;
}
