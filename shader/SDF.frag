#version 460
#extension GL_EXT_nonuniform_qualifier : require

#include "SDF.h"

layout(location = 0) in vec2 uv;
layout(location = 1) flat in uint offset;
layout(location = 2) flat in uint glyph_atlases_index;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D glyph_atlases[];

////////////////////////////////////////////////////////////////////////////////
//                                SDF shapes
////////////////////////////////////////////////////////////////////////////////

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
  switch (GetType(beg))
  {
    case Line_Partition:
    {
      vec2 p0 = GetPartitionP0(beg);
      vec2 p1 = GetPartitionP1(beg);
      beg += GetLinePartitionOffset();
      return sdf_line_partition(gl_FragCoord.xy, p0, p1);
    }
    case Bezier_Partition:
    {
      vec2 p0 = GetPartitionP0(beg);
      vec2 p1 = GetPartitionP1(beg);
      vec2 p2 = GetPartitionP2(beg);
      beg += GetBezierPartitionOffset();
      return sdf_bezier_partition(gl_FragCoord.xy, p0, p1, p2);
    }
  }
}

float get_distance(inout uint local_offset)
{
  switch (GetType(local_offset))
  {
    case Line:
    {
      vec2 p0 = GetP0(local_offset);
      vec2 p1 = GetP1(local_offset);
      local_offset += HeaderSize + 4;
      return sdSegment(gl_FragCoord.xy, p0, p1);
    }
    case Rectangle:
    {
      vec2 p0 = GetP0(local_offset);
      vec2 p1 = GetP1(local_offset);
      vec2 extent_div2 = (p1 - p0) * 0.5;
      vec2 center = p0 + extent_div2;
      local_offset += HeaderSize + 4;
      return sdBox(gl_FragCoord.xy - center, extent_div2);
    }
    case Triangle:
    {
      vec2 p0 = GetP0(local_offset);
      vec2 p1 = GetP1(local_offset);
      vec2 p2 = GetP2(local_offset);
      local_offset += HeaderSize + 6;
      return sdTriangle(gl_FragCoord.xy, p0, p1, p2);
    }
    case Polygon:
    {
      uint beg_offset = GetPolygonPointsBeginOffset(local_offset);
      uint count      = GetFirstValue(local_offset);
      float d         = sdPolygon(beg_offset, count, gl_FragCoord.xy);
      local_offset += HeaderSize + 1 + count * 2;
      return d;
    }
    case Circle:
    {
      vec2  center = GetP0(local_offset);
      float radius = GetThirdValueF(local_offset);
      local_offset += HeaderSize + 3;
      return sdCircle(gl_FragCoord.xy - center, radius);
    }
    case Bezier:
    {
      vec2 p0 = GetP0(local_offset);
      vec2 p1 = GetP1(local_offset);
      vec2 p2 = GetP2(local_offset);
      local_offset += HeaderSize + 6;
      return sdBezier(gl_FragCoord.xy, p0, p1, p2);
    }
    case Path:
    {
      float d     = -3.4028235e+38;
      uint  count = GetPathCount(local_offset);
      uint  begin = GetPathBeginOffset(local_offset);
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
      local_offset = begin;
      return d;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//                             main function
////////////////////////////////////////////////////////////////////////////////

void main()
{
  uint local_offset = offset;

  // glyph process
  if (GetType(local_offset) == Glyph)
  {
    // reference: https://computergraphics.stackexchange.com/questions/306/sharp-corners-with-signed-distance-fields-fonts
    // author: Detheroc
    float d = texture(glyph_atlases[nonuniformEXT(glyph_atlases_index)], uv).r - 0.5;
    float w = fwidth(d);
    float inner_alpha = clamp(d / w + 0.5, 0.0, 1.0);

    vec4 inner_color = GetInnerColor(local_offset);
    vec4 outer_color = GetOuterColor(local_offset);

    if (outer_color.a == 0.0)
      out_color = vec4(inner_color.rgb, inner_color.a * inner_alpha);
    else
    {
      // reference: https://www.redblobgames.com/x/2404-distance-field-effects/
      float outline_width = GetOutlineWidht(local_offset);
      float outer_alpha   = clamp((d + outline_width) / w + 0.5, 0.0, 1.0);

      if (inner_color.a == 0)
        inner_color = vec4(0);
      else
        inner_color = inner_color * inner_alpha;

      out_color = inner_color + (outer_color * (outer_alpha - inner_alpha));
    }
    return;
  }

  // other process
  float w = length(vec2(dFdxFine(gl_FragCoord.x), dFdyFine(gl_FragCoord.y)));

  out_color = GetColor(local_offset);
  uint  t   = GetThickness(local_offset);
  uint  op  = GetOperator(local_offset);
  float d   = get_distance(local_offset);

  while (op != Mix)
  {
    if (op == Min)
    {
      op = GetOperator(local_offset);
      out_color = GetColor(local_offset);
      t = GetThickness(local_offset);
      d = min(d, get_distance(local_offset));
    }
  }

  out_color = get_color(out_color, w, d, t);
}