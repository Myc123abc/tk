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
  // d
};
const uint Shape_Info_Size = 7;

void main()
{
  float w = length(vec2(dFdx(uv.x), dFdy(uv.y)));
  
  col = vec4(0.0);

  for (uint i = 0; i < pc.num; ++i)
  {
    uint shape_info_idx = pc.offset + i * Shape_Info_Size;
    ShapeInfo info;
    info.type   = pc.buf.data[shape_info_idx];
    info.offset = pc.buf.data[shape_info_idx + 1];
    info.num    = pc.buf.data[shape_info_idx + 2];
    info.color  = vec4(uintBitsToFloat(pc.buf.data[shape_info_idx + 3]),
                       uintBitsToFloat(pc.buf.data[shape_info_idx + 4]),
                       uintBitsToFloat(pc.buf.data[shape_info_idx + 5]),
                       uintBitsToFloat(pc.buf.data[shape_info_idx + 6]));

    uint point_idx = info.offset;
    float d;

    // line
    if (info.type == 0)
    {
      vec2 p0 = vec2(uintBitsToFloat(pc.buf.data[point_idx]),     uintBitsToFloat(pc.buf.data[point_idx + 1]));
      vec2 p1 = vec2(uintBitsToFloat(pc.buf.data[point_idx + 2]), uintBitsToFloat(pc.buf.data[point_idx + 3]));
      d = sdSegment(uv, p0, p1);
    }
    // box
    else if (info.type == 1)
    {
      vec2 p0 = vec2(uintBitsToFloat(pc.buf.data[point_idx]),     uintBitsToFloat(pc.buf.data[point_idx + 1]));
      vec2 p1 = vec2(uintBitsToFloat(pc.buf.data[point_idx + 2]), uintBitsToFloat(pc.buf.data[point_idx + 3]));
      vec2 extent_div2 = (p1 - p0) * 0.5;
      vec2 center = p0 + extent_div2;
      d = sdBox(uv - center, extent_div2);
    }
    // triangle
    else if (info.type == 2)
    {
      vec2 p0 = vec2(uintBitsToFloat(pc.buf.data[point_idx]),     uintBitsToFloat(pc.buf.data[point_idx + 1]));
      vec2 p1 = vec2(uintBitsToFloat(pc.buf.data[point_idx + 2]), uintBitsToFloat(pc.buf.data[point_idx + 3]));
      vec2 p2 = vec2(uintBitsToFloat(pc.buf.data[point_idx + 4]), uintBitsToFloat(pc.buf.data[point_idx + 5]));
      d = sdTriangle(uv, p0, p1, p2);
    }

    col = mix(info.color, col, smoothstep(0.0, w, d));
  }
}