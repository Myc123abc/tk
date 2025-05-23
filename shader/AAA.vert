#version 460

vec2 vertices[] =
{
  { -1, -1 },
  {  3, -1 },
  { -1,  3 },
};

layout(location = 0) out struct
{
  vec4 col;
  vec2 uv;
} Out;

// test data
int color = 0x282C34FF;

void main()
{
  gl_Position = vec4(vertices[gl_VertexIndex], 0, 1);
  Out.uv = vertices[gl_VertexIndex];

  float r = float((color >> 24) & 0xFF) / 255;
  float g = float((color >> 16) & 0xFF) / 255;
  float b = float((color >> 8 ) & 0xFF) / 255;
  float a = float((color      ) & 0xFF) / 255;
  Out.col = vec4(r, g, b, a);
}