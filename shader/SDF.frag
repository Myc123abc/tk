#version 460

#include "SDF.h"

layout(location = 0) in  vec2 uv;
layout(location = 0) out vec4 col;

layout(binding = 0) uniform sampler2D text_mask;

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

vec3 mix_color(vec3 color, vec3 background, float w, float d, uint t)
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
  ++shape_info_idx;
  switch (info.type)
  {
    case Line:
    {
      vec2 p0 = GetVec2(info.offset);
      vec2 p1 = GetVec2(info.offset + 2);
      return sdSegment(uv, p0, p1);
    }
    case Rectangle:
    {
      vec2 p0 = GetVec2(info.offset);
      vec2 p1 = GetVec2(info.offset + 2);
      vec2 extent_div2 = (p1 - p0) * 0.5;
      vec2 center = p0 + extent_div2;
      return sdBox(uv - center, extent_div2);
    }
    case Triangle:
    {
      vec2 p0 = GetVec2(info.offset);
      vec2 p1 = GetVec2(info.offset + 2);
      vec2 p2 = GetVec2(info.offset + 4);
      return sdTriangle(uv, p0, p1, p2);
    }
    case Polygon:
      return sdPolygon(info.offset, info.num, uv);
    case Circle:
    {
      vec2  center = GetVec2(info.offset);
      float radius = GetDataF(info.offset + 2);
      return sdCircle(uv - center, radius);
    }
    case Bezier:
    {
      vec2 p0 = GetVec2(info.offset);
      vec2 p1 = GetVec2(info.offset + 2);
      vec2 p2 = GetVec2(info.offset + 4);
      return sdBezier(uv, p0, p1, p2);
    }
    case Path:
    {
      float d = -3.4028235e+38;
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
      return d;
    }
  }
}

vec4 alpha_mix(vec3 color, float alpha, vec4 background)
{
  return vec4(color * alpha + background.rgb * (1 - alpha),
              alpha + background.a * (1 - alpha));
}

void main()
{
  float w = length(vec2(dFdxFine(uv.x), dFdyFine(uv.y)));
  
  col = vec4(1.0);
  vec3 anti_aliasing_color;

  while (shape_info_idx < pc.num)
  {
    ShapeInfo info = get_shape_info();

    if (info.type == Glyph)
    {
      ++shape_info_idx;
      //anti_aliasing_color = mix(col.rgb, info.color.rgb, texture(text_mask, uv));
      //col = alpha_mix(anti_aliasing_color, col.a, info.color); // FIXME:
      continue;
    }

    float d = get_distance(info);
    
    if (info.op == Mix)
    {
      anti_aliasing_color = mix_color(info.color.rgb, col.rgb, w, d, info.thickness);
      col = alpha_mix(anti_aliasing_color, info.color.a, col);
    }
    else if (info.op == Min) // TODO: currently, don't know how to handle two color and thickness, so just use first one's color and thickness.
    {
      ShapeInfo next_info = get_shape_info();
      
      d = min(d, get_distance(next_info));

      vec3 color = mix(info.color.rgb, next_info.color.rgb, smoothstep(0.0, w, d));

      anti_aliasing_color = mix_color(color, col.rgb, w, d, info.thickness);
      col = alpha_mix(anti_aliasing_color, info.color.a, col);
    }
  }
}