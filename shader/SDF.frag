#version 460

#include "SDF.h"

layout(location = 0) in  vec2 uv;
layout(location = 0) out vec4 col;

struct ShapeInfo
{
  uint type;
  uint offset;
  uint num; // DISCARD: unuse now
  vec4 color;
  uint thickness;
  // d
};
const uint Shape_Info_Size = 8;

vec4 mix_color(vec4 color, vec4 background, float w, float d, uint t)
{
  float value;
  if (t == 0)
    value = d;
  else if (t == 1)
    value = abs(d);
  else
  {
    if (d > 0.0)
      value = d;
    else
      value = -d - t + 1.0;
  }
  return mix(color, background, smoothstep(0.0, w, value));
}

#define GetData(idx)  pc.buf.data[idx]
#define GetDataF(idx) uintBitsToFloat(GetData(idx))
#define GetVec2(idx)  vec2(GetDataF(idx), GetDataF(idx + 1))
#define GetVec3(idx)  vec3(GetDataF(idx), GetDataF(idx + 1)，GetDataF(idx + 2))
#define GetVec4(idx)  vec4(GetDataF(idx), GetDataF(idx + 1), GetDataF(idx + 2), GetDataF(idx + 3))

#define Line      0
#define Rectangle 1
#define Triangle  2

void main()
{
  float w = length(vec2(dFdx(uv.x), dFdy(uv.y)));
  float d;

  col = vec4(0.0);

  for (uint i = 0; i < pc.num; ++i)
  {
    uint shape_info_idx = pc.offset + i * Shape_Info_Size;
    ShapeInfo info;
    info.type      = GetData(shape_info_idx);
    info.offset    = GetData(shape_info_idx + 1);
    info.num       = GetData(shape_info_idx + 2);
    info.color     = GetVec4(shape_info_idx + 3);
    info.thickness = GetData(shape_info_idx + 7);

    if (info.type == Line)
    {
      vec2 p0 = GetVec2(info.offset);
      vec2 p1 = GetVec2(info.offset + 2);
      d = sdSegment(uv, p0, p1);
    }
    else if (info.type == Rectangle)
    {
      vec2 p0 = GetVec2(info.offset);
      vec2 p1 = GetVec2(info.offset + 2);
      vec2 extent_div2 = (p1 - p0) * 0.5;
      vec2 center = p0 + extent_div2;
      d = sdBox(uv - center, extent_div2);
    }
    else if (info.type == Triangle)
    {
      vec2 p0 = GetVec2(info.offset);
      vec2 p1 = GetVec2(info.offset + 2);
      vec2 p2 = GetVec2(info.offset + 4);
      d = sdTriangle(uv, p0, p1, p2);
    }

    col = mix_color(info.color, col, w, d, info.thickness);
  }
}