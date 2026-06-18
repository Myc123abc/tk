
struct PSParam
{
  float4 pos : SV_Position;
  float2 uv  : TEXCOORD;
};

struct Constants
{
  float2 ext;
};

ConstantBuffer<Constants> constants : register(b0);
SamplerState              g_sampler : register(s0);
Texture2D                 img       : register(t0);

PSParam vs(uint id : SV_VertexID)
{
  float2 vertices[] =
  {
    { 0, 0 },
    { constants.ext.x, 0 },
    constants.ext,
    { 0, 0 },
    constants.ext,
    { 0, constants.ext.y },
  };

  float2 pos[] =
  {
    vertices[0] / constants.ext * float2(2, -2) + float2(-1, 1),
    vertices[1] / constants.ext * float2(2, -2) + float2(-1, 1),
    vertices[2] / constants.ext * float2(2, -2) + float2(-1, 1),
    vertices[3] / constants.ext * float2(2, -2) + float2(-1, 1),
    vertices[4] / constants.ext * float2(2, -2) + float2(-1, 1),
    vertices[5] / constants.ext * float2(2, -2) + float2(-1, 1),
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
  return img.SampleLevel(g_sampler, args.uv, 0);
}
