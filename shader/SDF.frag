#version 460

#include "SDF.h"

layout(location = 0) in  vec2 uv;
layout(location = 0) out vec4 col;

struct ShapeInfo
{
  uint type;
  uint offset;
  uint num;
  vec4 color;
  uint thickness;
  uint op;
};
const uint Shape_Info_Size = 9;

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

ShapeInfo get_shape_info(uint idx)
{
  ShapeInfo info;
  info.type      = GetData(idx);
  info.offset    = GetData(idx + 1);
  info.num       = GetData(idx + 2);
  info.color     = GetVec4(idx + 3);
  info.thickness = GetData(idx + 7);
  info.op        = GetData(idx + 8);
  return info;
}

float get_distance(ShapeInfo info)
{
  float d = 3.4028235e+38;
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
  else if (info.type == Circle)
  {
    vec2  center = GetVec2(info.offset);
    float radius = GetDataF(info.offset + 2);
    d = sdCircle(uv - center, radius);
  }
  else if (info.type == Bezier)
  {
    vec2 p0 = GetVec2(info.offset);
    vec2 p1 = GetVec2(info.offset + 2);
    vec2 p2 = GetVec2(info.offset + 4);
    d = sdBezier(uv, p0, p1, p2);
  }
  return d;
}

void main()
{
  float w = length(vec2(dFdxFine(uv.x), dFdyFine(uv.y)));

  col = vec4(0.0);

  for (uint i = 0; i < pc.num; ++i)
  {
    uint      idx  = pc.offset + i * Shape_Info_Size;
    ShapeInfo info = get_shape_info(idx);
    float     d    = get_distance(info);
    
    if (info.op == Mix)
    {
      col = mix_color(info.color, col, w, d, info.thickness);
    }
    else if (info.op == Min)
    {
      ShapeInfo next_info = get_shape_info(idx + Shape_Info_Size);
      
      d = min(d, get_distance(next_info));

      vec4 color = mix(info.color, next_info.color, smoothstep(0.0, w, d));

      col = mix_color(color, col, w, d, info.thickness);

      ++i;
    }
  }
}