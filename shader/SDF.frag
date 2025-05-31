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

void main()
{
  float w = length(vec2(dFdxFine(uv.x), dFdyFine(uv.y)));
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
    else if (info.type == Polygon)
    {
      d = sdPolygon(info.offset, info.num, uv);
    }

    col = mix_color(info.color, col, w, d, info.thickness);
  }
}