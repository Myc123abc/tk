#version 460

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 color;

layout(push_constant) uniform PushConstant
{
  mat4 model;
} transform_matrixs;

layout(location = 0) out vec3 out_color;

void main()
{
  gl_Position = transform_matrixs.model * vec4(position, 0.f, 1.f);
  out_color   = color;
}
