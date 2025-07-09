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
  }
}

void main()
{
  float w = length(vec2(dFdxFine(gl_FragCoord.x), dFdyFine(gl_FragCoord.y)));
  float d = get_distance();

  out_color = get_color(color, w, d, GetData(offset + 1));
}