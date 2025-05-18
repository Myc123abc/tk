#version 460

// use shader object extension need vert and frag have identical push constant
#include "2D.h"

layout(location = 0) in struct
{
  vec4 col;
  vec2 uv;
} In;

layout(location = 0) out vec4 col;

// TODO: should add image sampler

void main()
{
  col = In.col;
}