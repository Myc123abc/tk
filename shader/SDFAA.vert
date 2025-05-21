#version 460

vec2 vertices[] =
{
  { -1, -1 },
  {  3, -1 },
  { -1,  3 },
};

layout(location = 0) out vec2 uv;

void main()
{
  gl_Position = vec4(vertices[gl_VertexIndex], 0, 1);
  uv = vertices[gl_VertexIndex];
}