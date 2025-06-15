#version 460

#include "text_rendering.h"

layout(location = 0) in  vec2 uv;
layout(location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D font_atlas;

float median(float r, float g, float b) 
{
  return max(min(r, g), min(max(r, g), b));
}

float screenPxRange()
{
  const float pxRange = 2.0;
  vec2 unitRange = vec2(pxRange)/vec2(textureSize(font_atlas, 0));
  vec2 screenTexSize = vec2(1.0)/fwidth(uv);
  return max(0.5*dot(unitRange, screenTexSize), 1.0);
}

void main()
{
  vec3 msd = texture(font_atlas, uv).rgb;
  float sd = median(msd.r, msd.g, msd.b);
  float screenPxDistance = screenPxRange() * (sd - 0.5);
  float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
  // TODO: Add color in future, currently only white text.
  color = mix(vec4(0), vec4(1), opacity);
}