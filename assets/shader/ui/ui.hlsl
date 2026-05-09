#include "common.hlsl"
#include "sdf.hlsl"

#define FLT_MAX 0x7f7fffff
#define FLT_MIN 0xff7fffff

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
float get_d_line(float d, float t)
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
      value = d;
    else
      value = -d - t * 0.5;
  }
  return value;
}

float4 get_color_no_aa_line(float4 color, float d, float t)
{
  float value = get_d_line(d, t);
  if (value > 1e-5) return float4(0, 0, 0, 0);
  return color;
}

float4 get_color_no_aa(float4 color, float d, float t)
{
  float value = get_d(d, t);
  if (value > 1e-5) return float4(0, 0, 0, 0);
  return color;
}

float4 get_color_line(float4 color, float w, float d, float t)
{
  float value = get_d_line(d, t);
  if (value >= w) return float4(0, 0, 0, 0);

  // float alpha = 1.0 - smoothstep(0.0, w, value);
  float alpha = saturate(1.f - value * rcp(w));
  return float4(color.rgb, color.a * alpha);
}

float4 get_color(float4 color, float w, float d, float t)
{
  float value = get_d(d, t);
  if (value >= w) return float4(0, 0, 0, 0);

  // float alpha = 1.0 - smoothstep(0.0, w, value);
  float alpha = saturate(1.f - value * rcp(w));
  return float4(color.rgb, color.a * alpha);
}

float get_path_edge_dis(float2 pos, uint offset)
{
  DrawCmd cmd = path_cmds[offset];
  switch (cmd.type)
  {
  case add_path_line:
    return sdSegment(pos, cmd.p0, cmd.p1);

  case add_path_bezier_quad:
    return sdBezier(pos, cmd.p0, cmd.p1, cmd.p2);

  case add_path_arc:
    return sdArc(pos, cmd.p0, cmd.p1, cmd.p2);
  }
  return 0;
}

int get_path_winding(float2 pos, uint offset)
{
  DrawCmd cmd = path_cmds[offset];
  switch (cmd.type)
  {
  case add_path_line:
    return path_line_winding(pos, cmd.p0, cmd.p1);

  case add_path_bezier_quad:
    return path_bezier_winding(pos, cmd.p0, cmd.p1, cmd.p2);

  case add_path_arc:
    return path_arc_winding(pos, cmd.p0, cmd.p1, cmd.p2);
  }
  return 0;
}

float get_path_stroke_dis(float2 pos, uint beg, uint count)
{
  float d = FLT_MAX;
  for (uint i = 0; i < count; ++i)
    d = min(d, get_path_edge_dis(pos, beg + i));
  return d;
}

float get_path_fill_dis(float2 pos, uint beg, uint count)
{
  float d = FLT_MAX;
  int winding = 0;
  for (uint i = 0; i < count; ++i)
  {
    d = min(d, get_path_edge_dis(pos, beg + i));
    winding += get_path_winding(pos, beg + i);
  }
  return winding == 0 ? d : -d;
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
      return get_color_no_aa_line(cmd.color, d, cmd.thickness);
    else
      return get_color_line(cmd.color, w, d, cmd.thickness);
  }

  case add_arc:
  {
    float d = sdArc(pos, cmd.p0, cmd.p1, cmd.p2);
    return get_color_line(cmd.color, w, d, cmd.thickness);
  }

  case add_bezier_quad:
  {
    float d = sdBezier(pos, cmd.p0, cmd.p1, cmd.p2);
    return get_color_line(cmd.color, w, d, cmd.thickness);
  }

  case add_path:
  {
    uint beg   = asuint(cmd.p0.x);
    uint count = asuint(cmd.p0.y);
    float d = cmd.thickness > 0 ? get_path_stroke_dis(pos, beg, count) : get_path_fill_dis(pos, beg, count);
    return get_color(cmd.color, w, d, cmd.thickness);
  }
  }
  return float4(0, 0, 0, 0);
}

float get_sd(float2 pos, DrawCmd cmd)
{
  switch (cmd.type)
  {
  case add_rect:
  {
    float2 extent_div2 = (cmd.p1 - cmd.p0) * 0.5;
    float2 center = cmd.p0 + extent_div2;
    return sdBox(pos - center, extent_div2);
  }

  case add_triangle:
    return sdTriangle(pos, cmd.p0, cmd.p1, cmd.p2);

  case add_circle:
  {
    float2 center = cmd.p0;
    float radius = cmd.p1.x;
    return sdCircle(pos - center, radius);
  }

  case add_line:
    return sdSegment(pos, cmd.p0, cmd.p1);

  case add_arc:
    return sdArc(pos, cmd.p0, cmd.p1, cmd.p2);

  case add_bezier_quad:
    return sdBezier(pos, cmd.p0, cmd.p1, cmd.p2);

  case add_path:
  {
    uint beg   = asuint(cmd.p0.x);
    uint count = asuint(cmd.p0.y);
    return get_path_fill_dis(pos, beg, count);
  }
  }
  return FLT_MAX;
}

float4 ps(float4 pos4 : SV_POSITION) : SV_TARGET
{
  float2 pos = pos4.xy;

  uint2 tile_idxs = pos / constants.tile_size;
  uint  tile_idx  = tile_idxs.y * constants.tile_count.x + tile_idxs.x;
  Tile  tile      = tiles[tile_idx];

  float4 color = float4(0, 0, 0, 0);

  float w = length(float2(ddx_fine(pos.x), ddy_fine(pos.y)));

  int    op    = op_none;

  for (uint i = 0; i < tile.count; ++i)
  {
    DrawCmd cmd = cmds[cmd_idxs[tile.beg + i]];

    if (cmd.op == op_uni)
    {
      float  min_d = FLT_MAX;
      float4 min_col;
      float  min_t;

      float d = get_sd(pos, cmd);
      if (d < min_d)
      {
        min_d   = d;
        min_col = cmd.color;
        min_t   = cmd.thickness;
      }

      uint cnt = cmd.uni_cnt;
      for (uint j = 1; j < cnt; ++j)
      {
        cmd = cmds[cmd_idxs[tile.beg + i + j]];
        min_d = min(min_d, get_sd(pos, cmd));
      }

      color = blend(get_color(min_col, w, min_d, min_t), color);

      i += cnt - 1;
      continue;
    }

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
