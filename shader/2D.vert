#version 460
#extension GL_EXT_buffer_reference : require

// TODO: check memory align problem if no image

struct Vertex
{
  vec2  pos;
  vec2  uv;
  uint  col;
};

layout(std430, buffer_reference) readonly buffer Vertices 
{
  Vertex data[];
};

layout(push_constant) uniform PushConstant
{
  Vertices vertices;
  vec2     scale;
  vec2     translate;
} pc;

layout(location = 0) out struct
{
  vec4 col;
  vec2 uv;
} Out;

void main()
{
  // get vertex
  Vertex vertex = pc.vertices.data[gl_VertexIndex];
  
  // set vertex position
  gl_Position   = vec4(vertex.pos * pc.scale + pc.translate, 0, 1);

  // get color
  float r = vertex.col >> 16 & 0xFF / 255;
  float g = vertex.col >> 8  & 0xFF / 255;
  float b = vertex.col       & 0xFF / 255;
  float a = vertex.col >> 24 & 0xFF / 255;
  Out.col = vec4(r, g, b, a);
  
  // set uv
  Out.uv  = vertex.uv;
}
