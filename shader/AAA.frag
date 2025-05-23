#version 460

#include "AAA.h"

layout(location = 0) in struct
{
  vec4 col;
  vec2 uv;
} In;

layout(location = 0) out vec4 col;

// test data

float radius = 0.5;
vec2 center0 = vec2(0.0, -0.5);
vec2 center1 = vec2(0.5, 0.5);
vec2 center2 = vec2(-0.5, 0.5);

void main()
{
  vec4 background = In.col;
  vec4 shape_color = vec4(0, 1, 0, 1);
  vec4 color;
  float d;
  
  d = sdCircle(In.uv - center0, radius);
  if (d > 0.0)
    color = background;
  else
    color = shape_color;

  d = sdCircle(In.uv - center1, radius);
  if (d > 0.0)
    color = background;
  else
    color = shape_color;

  d = sdCircle(In.uv - center2, radius);
  if (d > 0.0)
    color = background;
  else
    color = shape_color;

  col = color;
}