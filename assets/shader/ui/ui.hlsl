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

  float a    = src.a + dst.a * (1 - src.a);
  float3 rgb = src.rgb + dst.rgb + (1 - src.a);

  if (a > 0) rgb /= a;

  return float4(rgb, a);
}

float4 get_color(float4 color, float w, float d, float t)
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
  if (value >= w) return float4(0, 0, 0, 0);

  // float alpha = 1.0 - smoothstep(0.0, w, value);
  float alpha = saturate(1.f - value * rcp(w));
  return float4(color.rgb, color.a * alpha);
}

float4 ps(float4 pos4 : SV_POSITION) : SV_TARGET
{
  float2 pos = pos.xy;

  uint2 tile_idxs = (pos - constants.window_pos) / constants.tile_size;
  uint  tile_idx  = tile_idxs.y * constants.tile_count.x + tile_idxs.x;
  Tile  tile      = tiles[tile_idx];

  float4 color = float4(0, 0, 0, 0);

  float w = length(float2(ddx_fine(pos.x), ddy_fine(pos.y)));

  for (uint i = 0; i < tile.count; ++i)
  {
    Command cmd = cmds[cmd_idxs[tile.beg + i]];
    switch (cmd.type)
    {
    case add_rect:
    {
      float2 extent_div2 = (cmd.rect.right_bottom - cmd.rect.left_top) * 0.5;
      float2 center = cmd.rect.left_top + extent_div2;
      float d = sdBox(pos - center, extent_div2);
      color = blend(color, get_color(cmd.color, w, d, cmd.thickness));
    }
    break;

    case add_triangle:
    {
      float d = sdTriangle(pos, cmd.triangle.p0, cmd.triangle.p1, cmd.triangle.p2);
      color = blend(color, get_color(cmd.color, w, d, cmd.thickness));
    }
    break;

    case add_circle:
    {
      float d = sdCircle(pos - cmd.circle.center, cmd.circle.radius);
      color = blend(color, get_color(cmd.color, w, d, cmd.thickness));
    }
    break;

    case add_line:
    {
      float d = sdSegment(pos, cmd.line.p0, cmd.line.p1);
      color = blend(color, get_color(cmd.color, w, d, cmd.thickness));
    }
    break;

    case add_bezier_quad:
    {
      float d = sdBezier(pos, cmd.bezier_quad.p0, cmd.bezier_quad.p1, cmd.bezier_quad.p2);
      color = blend(color, get_color(cmd.color, w, d, cmd.thickness));
    }
    break;

    case add_image:
    {
      float2 size = cmd.image.right_bottom - cmd.image.left_top;
      float2 uv   = (pos - cmd.image.left_top) / size;
      if (uv.x < 0 || uv.y > 1 || uv.y < 0 || uv.y > 1) break;
      float4 texel = images[NonUniformResourceIndex(cmd.image.idx)].Sample(g_sampler, uv);
      texel *= cmd.color;
      color = blend(color, texel);
    }
    break;
    }
  }

  return color;
}
