#version 460

#include "SDF.h"

layout(location = 0) in  vec2 uv;
layout(location = 1) in  vec4 color;
layout(location = 0) out vec4 out_color;

float get_distance()
{
  uint shape_type = GetData(0);
  switch (shape_type)
  {
    case Rectangle:
    {
      vec2 p0 = GetVec2(1);
      vec2 p1 = GetVec2(3);
      vec2 extent_div2 = (p1 - p0) * 0.5;
      vec2 center = p0 + extent_div2;
      return sdBox(gl_FragCoord.xy - center, extent_div2);
    }
  }
}

void main()
{
  float d = get_distance():
  out_color = color;
}