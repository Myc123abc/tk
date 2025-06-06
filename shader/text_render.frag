#version 460

layout(location = 0) in  vec2 uv;
layout(location = 0) out vec4 col;

layout(binding = 0) uniform sampler2D ft_image;

void main()
{
  col = texture(ft_image, uv);
}