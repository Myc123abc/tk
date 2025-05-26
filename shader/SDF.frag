#version 460
#extension GL_EXT_buffer_reference : require

layout(location = 0) in  vec2 uv;
layout(location = 0) out vec4 col;

#include "SDF.h"

layout(std430, buffer_reference) readonly buffer Buffer
{
  uint data[];
};

/*
  buffer:      points | shape_infos

  shape_infos: offset | num | color
*/

layout(push_constant) uniform PushConstant
{
  Buffer buf;
  uint   offset; // offset of shape infos
  uint   num;    // number of shape infos
} pc;

struct ShapeInfo
{
  uint offset;
  uint num;
  vec4 color;
  // d
  // type
};

const uint Shape_Info_Size = 6;
const uint Point_Size      = 2;
const uint Line_Point_Num  = 2;

void main()
{
  float w = length(vec2(dFdx(uv.x), dFdy(uv.y)));
  
  col = vec4(0.0);

  for (uint i = 0; i < pc.num; ++i)
  {
    uint shape_info_idx = pc.offset + i * Shape_Info_Size;
    ShapeInfo info;
    info.offset = pc.buf.data[shape_info_idx];
    info.num    = pc.buf.data[shape_info_idx + 1];
    info.color  = vec4(uintBitsToFloat(pc.buf.data[shape_info_idx + 2]),
                       uintBitsToFloat(pc.buf.data[shape_info_idx + 3]),
                       uintBitsToFloat(pc.buf.data[shape_info_idx + 4]),
                       uintBitsToFloat(pc.buf.data[shape_info_idx + 5]));

    uint point_idx = info.offset;
    vec2 p0 = vec2(uintBitsToFloat(pc.buf.data[point_idx]),     uintBitsToFloat(pc.buf.data[point_idx + 1]));
    vec2 p1 = vec2(uintBitsToFloat(pc.buf.data[point_idx + 2]), uintBitsToFloat(pc.buf.data[point_idx + 3]));

    float d = sdSegment(uv, p0, p1);

    col = mix(info.color, col, smoothstep(0.0, w, d));
  }
}