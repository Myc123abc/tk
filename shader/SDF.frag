#version 460

#include "SDF.h"

layout(location = 0) in vec2 uv;
layout(location = 1) flat in uint offset;

layout(location = 0) out vec4 out_color;

uint g_offset;

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

#define HeaderSize 7

#define GetTypeAt(x) GetData(x)
#define GetP0At(x)   GetVec2(x + HeaderSize)
#define GetP1At(x)   GetVec2(x + HeaderSize + 2)
#define GetP2At(x)   GetVec2(x + HeaderSize + 4)

#define GetPartitionP0At(x) GetVec2(x + 1)
#define GetPartitionP1At(x) GetVec2(x + 3)
#define GetPartitionP2At(x) GetVec2(x + 5)

#define GetType() GetTypeAt(g_offset)
#define GetP0()   GetP0At(g_offset)
#define GetP1()   GetP1At(g_offset)
#define GetP2()   GetP2At(g_offset)

#define GetPolygonPointsBeginOffset() g_offset + HeaderSize + 1

#define GetFirstValue()  GetData(g_offset + HeaderSize)
#define GetThirdValueF() GetDataF(g_offset + HeaderSize + 2)

#define GetPathCount()             GetData(g_offset + HeaderSize)
#define GetPathBeginOffset()       g_offset + HeaderSize + 1
#define GetLinePartitionOffset()   5
#define GetBezierPartitionOffset() 7

#define GetColor()     GetVec4(g_offset + 1)
#define GetThickness() GetData(g_offset + 5)
#define GetOperator()  GetData(g_offset + 6)

float get_distance_parition(inout uint beg)
{
  switch (GetTypeAt(beg))
  {
    case Line_Partition:
    {
      vec2 p0 = GetPartitionP0At(beg);
      vec2 p1 = GetPartitionP1At(beg);
      beg += GetLinePartitionOffset();
      return sdf_line_partition(gl_FragCoord.xy, p0, p1);
    }
    case Bezier_Partition:
    {
      vec2 p0 = GetPartitionP0At(beg);
      vec2 p1 = GetPartitionP1At(beg);
      vec2 p2 = GetPartitionP2At(beg);
      beg += GetBezierPartitionOffset();
      return sdf_bezier_partition(gl_FragCoord.xy, p0, p1, p2);
    }
  }
}

float get_distance()
{
  switch (GetType())
  {
    case Line:
    {
      vec2 p0 = GetP0();
      vec2 p1 = GetP1();
      return sdSegment(gl_FragCoord.xy, p0, p1);
    }
    case Rectangle:
    {
      vec2 p0 = GetP0();
      vec2 p1 = GetP1();
      vec2 extent_div2 = (p1 - p0) * 0.5;
      vec2 center = p0 + extent_div2;
      return sdBox(gl_FragCoord.xy - center, extent_div2);
    }
    case Triangle:
    {
      vec2 p0 = GetP0();
      vec2 p1 = GetP1();
      vec2 p2 = GetP2();
      return sdTriangle(gl_FragCoord.xy, p0, p1, p2);
    }
    case Polygon:
    {
      return sdPolygon(GetPolygonPointsBeginOffset(), GetFirstValue(), gl_FragCoord.xy);
    }
    case Circle:
    {
      vec2  center = GetP0();
      float radius = GetThirdValueF();
      return sdCircle(gl_FragCoord.xy - center, radius);
    }
    case Bezier:
    {
      vec2 p0 = GetP0();
      vec2 p1 = GetP1();
      vec2 p2 = GetP2();
      return sdBezier(gl_FragCoord.xy, p0, p1, p2);
    }
    case Path:
    {
      float d     = -3.4028235e+38;
      uint  count = GetPathCount();
      uint  begin = GetPathBeginOffset();
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
  g_offset = offset;

  float w = length(vec2(dFdxFine(gl_FragCoord.x), dFdyFine(gl_FragCoord.y)));
  float d = get_distance();

  uint op = GetOperator();
  while (op != Mix)
  {
    if (op == Min)
    {
      //g_offset += GetOffset();
      d = min(d, get_distance());
    }
  }

  out_color = get_color(GetColor(), w, d, GetThickness());
}