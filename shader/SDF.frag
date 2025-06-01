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
      uint shape_info_idx  = 0;
#define GetShapeInfoOffset() (pc.offset + shape_info_idx * Shape_Info_Size)

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

ShapeInfo get_shape_info()
{
  uint offset = GetShapeInfoOffset();
  ShapeInfo info;
  info.type      = GetData(offset);
  info.offset    = GetData(offset + 1);
  info.num       = GetData(offset + 2);
  info.color     = GetVec4(offset + 3);
  info.thickness = GetData(offset + 7);
  info.op        = GetData(offset + 8);
  return info;
}

float get_distance_parition(ShapeInfo info)
{
  float d;
  ++shape_info_idx;
  if (info.type == Line_Partition)
  {
    vec2 p0 = GetVec2(info.offset);
    vec2 p1 = GetVec2(info.offset + 2);
    d = sdf_line_partition(uv, p0, p1);
  }
  else if (info.type == Bezier_Partition)
  {
    vec2 p0 = GetVec2(info.offset);
    vec2 p1 = GetVec2(info.offset + 2);
    vec2 p2 = GetVec2(info.offset + 4);
    d = sdf_bezier_partition(uv, p0, p1, p2);
  }
  return d;
}

float get_distance(ShapeInfo info)
{
  float d;
  ++shape_info_idx;
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
  else if (info.type == Path)
  {
    d = -3.4028235e+38;
    for (uint i = 0; i < info.num; ++i)
    {
      ShapeInfo partition_info = get_shape_info();
      float partition_distance = get_distance_parition(partition_info);
      float distance = max(d, partition_distance);
      
      // aliasing problem:
      // when two line segment in same line, such as (0,0)(50,50) to (50,50)(100,100)
      // max(d0,d1) will lead aliasing problem
      // so use min(abs(d0),abs(d1)) to resolve
      // well min's way can only use for 1-pixel case,
      // so for filled and thickness wireform we use max still,
      // and use min on bround, perfect! (I spent half day to resolve... my holiday...)
      if (distance > 0.0)
        d = min(abs(d), abs(partition_distance));
      else
        d = distance;
    }
  }
  return d;
}

void main()
{
  float w = length(vec2(dFdxFine(uv.x), dFdyFine(uv.y)));
  
  col = vec4(0.0);

  while (shape_info_idx < pc.num)
  {
    ShapeInfo info = get_shape_info();
    float     d    = get_distance(info);
    
    if (info.op == Mix)
    {
      col = mix_color(info.color, col, w, d, info.thickness);
    }
    else if (info.op == Min)
    {
      ShapeInfo next_info = get_shape_info();
      
      d = min(d, get_distance(next_info));

      vec4 color = mix(info.color, next_info.color, smoothstep(0.0, w, d));

      col = mix_color(color, col, w, d, info.thickness);
    }
  }
}