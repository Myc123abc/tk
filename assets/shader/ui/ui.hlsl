#include "common.hlsl"
#include "sdf.hlsl"

float4 vs(uint id : SV_VertexID) : SV_POSITION
{
  float2 vertices[] =
  {
    constants.window_pos,
    { constants.window_pos.x + constants.window_extent.x, constants.window_pos.y },
    constants.window_pos + constants.window_extent,
    constants.window_pos,
    constants.window_pos + constants.window_extent,
    { constants.window_pos.x, constants.window_pos.y + constants.window_extent.y },
  };
  return float4(vertices[id] / constants.render_target_extent * float2(2, -2) + float2(-1, 1), 0, 1);
}

float4 blend(float4 src, float4 dst)
{
  src.rgb *= src.a;
  dst.rgb *= dst.a;

  float a = src.a + dst.a * (1.0 - src.a);
  float3 rgb = src.rgb + dst.rgb * (1.0 - src.a);

  if (a > 0.0) rgb /= a;

  return float4(rgb, a);
}

float get_d(float d, float t)
{
  float value;
  if (t == 0)
    value = d;
  else if (t == 1)
    value = abs(d);
  else
  {
    if (d > 0.0)
      value = d - t * 0.5 + 1;
    else
      value = d;
  }
  return value;
}

float4 get_color_no_aa(float4 color, float d, float t)
{
  float value = get_d(d, t);
  if (value > 1e-4) return float4(0, 0, 0, 0);
  return color;
}

float4 get_color(float4 color, float w, float d, float t)
{
  float value = get_d(d, t);
  if (value >= w) return float4(0, 0, 0, 0);

  // float alpha = 1.0 - smoothstep(0.0, w, value);
  float alpha = saturate(1.f - value * rcp(w));
  return float4(color.rgb, color.a * alpha);
}

float get_path_dis(float2 pos, uint offset)
{
  DrawCmd cmd = path_cmds[offset];
  switch (cmd.type)
  {
  case add_path_line:
    return sdf_line_partition(pos, cmd.p0, cmd.p1);

  case add_path_bezier_quad:
    return sdf_bezier_partition(pos, cmd.p0, cmd.p1, cmd.p2);

  case add_path_arc:
    return sdArcAngles(pos, cmd.p0, cmd.p1.x, cmd.p1.y, cmd.p2.x, true, cmd.thickness * 0.5);
  }
  return 0;
}

float get_path_dis(float2 pos, uint beg, uint count)
{
  float d = asfloat(0xff7fffff);
  for (uint i = 0; i < count; ++i)
  {
    float path_dis = get_path_dis(pos, beg + i);
    float dis = max(d, path_dis);

    // aliasing problem:
    // when two line segment in same line, such as (0,0)(50,50) to (50,50)(100,100)
    // max(d0,d1) will lead aliasing problem
    // so use min(abs(d0),abs(d1)) to resolve
    // well min's way can only use for 1-pixel case,
    // so for filled and thickness wireform we use max still,
    // and use min on bround, perfect! (I spent half day to resolve... my holiday...)
    if (dis > 0)
      d = min(abs(d), abs(path_dis));
    else
      d = dis;
  }
  return d;
}

float4 get_color(float2 pos, DrawCmd cmd, float w)
{
  switch (cmd.type)
  {
  case add_rect:
  {
    float2 extent_div2 = (cmd.p1 - cmd.p0) * 0.5;
    float2 center = cmd.p0 + extent_div2;
    float d = sdBox(pos - center, extent_div2);
    return get_color_no_aa(cmd.color, d, cmd.thickness);
  }

  case add_triangle:
  {
    float d = sdTriangle(pos, cmd.p0, cmd.p1, cmd.p2);
    return get_color(cmd.color, w, d, cmd.thickness);
  }

  case add_circle:
  {
    float2 center = cmd.p0;
    float radius = cmd.p1.x;
    float d = sdCircle(pos - center, radius);
    return get_color(cmd.color, w, d, cmd.thickness);
  }

  case add_line:
  {
    float d = sdSegment(pos, cmd.p0, cmd.p1);
    if (asuint(cmd.p2.x) == 1)
      return get_color_no_aa(cmd.color, d, cmd.thickness);
    else
      return get_color(cmd.color, w, d, cmd.thickness);
  }

  case add_bezier_quad:
  {
    float d = sdBezier(pos, cmd.p0, cmd.p1, cmd.p2);
    return get_color(cmd.color, w, d, cmd.thickness);
  }

  case add_path:
  {
    uint beg   = asuint(cmd.p0.x);
    uint count = asuint(cmd.p0.y);
    float d = get_path_dis(pos, beg, count);
    return get_color(cmd.color, w, d, cmd.thickness);
  }
  }
  return float4(0, 0, 0, 0);
}

float4 ps(float4 pos4 : SV_POSITION) : SV_TARGET
{
  float2 pos = pos4.xy;

  uint2 tile_idxs = pos / constants.tile_size;
  uint  tile_idx  = tile_idxs.y * constants.tile_count.x + tile_idxs.x;
  Tile  tile      = tiles[tile_idx];

  float4 color = float4(0, 0, 0, 0);

  float w = length(float2(ddx_fine(pos.x), ddy_fine(pos.y)));

  for (uint i = 0; i < tile.count; ++i)
  {
    DrawCmd cmd = cmds[cmd_idxs[tile.beg + i]];

    if (cmd.type == add_image)
    {
      float2 size = cmd.p1 - cmd.p0;
      float2 uv = (pos - cmd.p0) / size;

      if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1)
        continue;

      float4 texel = images[NonUniformResourceIndex(asint(cmd.p2.x))].Sample(g_sampler, uv);
      texel *= cmd.color;
      color = blend(texel, color);
    }
    else
      color = blend(get_color(pos, cmd, w), color);
  }

  return float4(color.rgb * color.a, color.a);
}
