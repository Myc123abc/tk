#version 460

#include "SDFAA.h"

layout(location = 0) in  vec2 uv;
layout(location = 0) out vec4 col;

vec2 a = vec2(0, -0.5);
vec2 b = vec2(0.5, 0.5);
vec2 c = vec2(-0.5, 0.5);

void main()
{
  int v = 0x282C34FF;
  float r = float((v >> 24) & 0xFF) / 255;
  float g = float((v >> 16) & 0xFF) / 255;
  float bb = float((v >> 8 ) & 0xFF) / 255;
  vec3 background = vec3(r, g, bb);
  vec3 shape_color = vec3(0, 1, 0);

  //a = vec2(-0.5, -1);
  //b = vec2(0, 0);
  //c = vec2(-1, 0);

  float d = sdTriangle(uv, a, b, c);
  vec2 center = vec2(0,0);
  //d = sdCircle(uv - center, 0.5);
  vec3 color = d > 0.0 ? background : shape_color;

  //color *= exp(d);
  //color *= exp(2.0 * d);
  //color *= exp(-2.0 * abs(d));
  //color *= 1.0 - exp(-2.0 * abs(d));

  //color = color * 0.8 + color * 0.2;
  //color = color * 0.8 + color * 0.2 * sin(d);
  //color = color * 0.8 + color * 0.2 * sin(50 * d);
  //color = color * 0.8 + color * 0.2 * sin(50 * d - 4.0 * time);

  float factor = fwidth(d);

  // for wireform
  // TODO: when less 0.0, no aa
  // TODO: control thickness
  //if (d < -0.02)
  //  //color = background;
  //  color = mix(shape_color, background, smoothstep(0.0, factor, abs(d)));

  //color = mix(shape_color, color, step(factor, d));
  //color = mix(shape_color, color, step(factor, abs(d)));
  //color = mix(shape_color, color, smoothstep(0.0, factor, abs(d)));

  //color = mix(vec4(1, 1, 1, 1), color, abs(d));
  //color = mix(vec4(1, 1, 1, 1), color, 2.0 * abs(d));
  //color = mix(vec4(1, 1, 1, 1), color, 4.0 * abs(d));

  col = vec4(color, 1.0);
}