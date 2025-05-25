#version 460

layout(location = 0) in  vec2 uv;
layout(location = 0) out vec4 col;

void main()
{
  col = vec4(uv.x, uv.y, 0, 1);
}