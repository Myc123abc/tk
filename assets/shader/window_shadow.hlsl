#include "root_signature.h"
#include "sdf.h"

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
  float2 p0 = constants.window_pos + float2(constants.shadow_thickness, constants.shadow_thickness);
  float2 p1 = constants.window_pos + constants.window_extent - float2(constants.shadow_thickness, constants.shadow_thickness);

  if (pos.x > p0.x && pos.x < p1.x && pos.y > p0.y && pos.y < p1.y)
    discard;

  float2 extent_div2 = (p1 - p0) * 0.5;
  float2 center = p0 + extent_div2;
  float d = sdBox(pos.xy - center, extent_div2);

  float shadow = smoothstep(constants.shadow_radius, constants.shadow_radius - constants.shadow_softness, d);

  return float4(constants.shadow_color, shadow);
}
