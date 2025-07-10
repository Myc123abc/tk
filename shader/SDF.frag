#version 460

#include "SDF.h"

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;
layout(location = 2) flat in uint offset;

layout(location = 0) out vec4 out_color;

vec4 get_color(vec4 color, float w, float d, uint t)
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
  if (value >= w) discard;
  float alpha = 1.0 - smoothstep(0.0, w, value);
  return vec4(color.rgb, color.a * alpha);
}

float get_distance_parition(inout uint beg)
{
  switch (GetData(beg + 0))
  {
    case Line_Partition:
    {
      vec2 p0 = GetVec2(beg + 2);
      vec2 p1 = GetVec2(beg + 4);
      beg += 6;
      return sdf_line_partition(gl_FragCoord.xy, p0, p1);
    }
    case Bezier_Partition:
    {
      vec2 p0 = GetVec2(beg + 2);
      vec2 p1 = GetVec2(beg + 4);
      vec2 p2 = GetVec2(beg + 6);
      beg += 8;
      return sdf_bezier_partition(gl_FragCoord.xy, p0, p1, p2);
    }
  }
}

float get_distance()
{
  switch (GetData(offset + 0))
  {
    case Line:
    {
      vec2 p0 = GetVec2(offset + 2);
      vec2 p1 = GetVec2(offset + 4);
      return sdSegment(gl_FragCoord.xy, p0, p1);
    }
    case Rectangle:
    {
      vec2 p0 = GetVec2(offset + 2);
      vec2 p1 = GetVec2(offset + 4);
      vec2 extent_div2 = (p1 - p0) * 0.5;
      vec2 center = p0 + extent_div2;
      return sdBox(gl_FragCoord.xy - center, extent_div2);
    }
    case Triangle:
    {
      vec2 p0 = GetVec2(offset + 2);
      vec2 p1 = GetVec2(offset + 4);
      vec2 p2 = GetVec2(offset + 6);
      return sdTriangle(gl_FragCoord.xy, p0, p1, p2);
    }
    case Polygon:
    {
      return sdPolygon(offset + 3, GetData(offset + 2), gl_FragCoord.xy);
    }
    case Circle:
    {
      vec2  center = GetVec2(offset + 2);
      float radius = GetDataF(offset + 4);
      return sdCircle(gl_FragCoord.xy - center, radius);
    }
    case Bezier:
    {
      vec2 p0 = GetVec2(offset + 2);
      vec2 p1 = GetVec2(offset + 4);
      vec2 p2 = GetVec2(offset + 6);
      return sdBezier(gl_FragCoord.xy, p0, p1, p2);
    }
    case Path:
    {
      float d     = -3.4028235e+38;
      uint  count = GetData(offset + 2);
      uint  begin = offset + 3;
      for (uint i = 0; i < count; ++i)
      {
        float partition_distance = get_distance_parition(begin);
        float distance           = max(d, partition_distance);

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

void main()
{
  float w = length(vec2(dFdxFine(gl_FragCoord.x), dFdyFine(gl_FragCoord.y)));
  float d = get_distance();

  out_color = get_color(color, w, d, GetData(offset + 1));
}