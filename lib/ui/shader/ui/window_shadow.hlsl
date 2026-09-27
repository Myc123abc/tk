#include "common.hlsl"
#include "sdf.hlsl"

float4 vs(uint id : SV_VertexID) : SV_Position
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

float4 ps(float4 pos : SV_Position) : SV_TARGET
{
  // get window rect
  float2 p0 = constants.window_pos + float2(constants.shadow_thickness, constants.shadow_thickness);
  float2 p1 = constants.window_pos + constants.window_extent - float2(constants.shadow_thickness, constants.shadow_thickness);

  // discard window content region
  if (pos.x > p0.x && pos.x < p1.x && pos.y > p0.y && pos.y < p1.y)
    return float4(0, 0, 0, 0);

  // calculate shadow color
  float4 color = float4(0, 0, 0, 0);

  if (constants.shadow_radius > 0)
  {
    float2 extent_div2 = (p1 - p0) * 0.5;
    float2 center      = p0 + extent_div2;
    float  d           = sdBox(pos.xy - center, extent_div2);
    float  shadow      = smoothstep(constants.shadow_radius, constants.shadow_radius - constants.shadow_softness, d);
    color = float4(constants.shadow_color, shadow);
  }

  // draw wireframe
  if (constants.draw_wireframe == 1)
  {
    // if on wireframe
    if (pos.x > p0.x - 1 && pos.x < p1.x + 1 && pos.y > p0.y - 1 && pos.y < p1.y + 1)
    {
      if (constants.shadow_radius > 0)
        color = lerp(color, constants.wireframe_color, constants.wireframe_color.a);
      else
        color = constants.wireframe_color;
    }
  }

  return color;
}
