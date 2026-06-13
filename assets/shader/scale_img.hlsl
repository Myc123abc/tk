
struct PSParam
{
  float4 pos : SV_Position;
  float2 uv  : TEXCOORD;
};

struct Constants
{
  uint2 ext;
};

ConstantBuffer<Constants> constants : register(b0);
SamplerState              g_sampler : register(s0);
Texture2D                 img       : register(t0);

PSParam vs(uint id : SV_VertexID)
{
  uint2 extent;
  img.GetDimensions(extent.x, extent.y);

  float2 ext = (float2)extent / constants.ext * 2 - 1;

  float2 pos[] =
  {
    { -1, 1 },
    { ext.x, 1 },
    ext,
    { -1, 1 },
    ext,
    { -1, ext.y },
  };

  float2 uvs[] =
  {
    { 0, 0 },
    { 1, 0 },
    { 1, 1 },
    { 0, 0 },
    { 1, 1 },
    { 0, 1 },
  };

  PSParam ps;
  ps.pos = float4(pos[id], 0, 1);
  ps.uv  = uvs[id];
  return ps;
}

float4 ps(PSParam args) : SV_TARGET
{
  return img.Sample(g_sampler, args.uv);
}
