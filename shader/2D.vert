#version 460
#extension GL_EXT_buffer_reference : require

// TODO: check memory align problem if no image

struct Vertex
{
  vec2  pos;
  vec2  uv;
  vec4  col;
};

layout(std430, buffer_reference, buffer_reference_align = 32) readonly buffer Vertices 
{
  Vertex data[];
};

layout(push_constant) uniform PushConstant
{
  vec2     scale;
  vec2     translate;
  Vertices vertices;
} pc;

layout(location = 0) out struct
{
  vec4 col;
  vec2 uv;
} Out;

void main()
{
  Vertex vertex = pc.vertices.data[gl_VertexIndex];
  gl_Position   = vec4(vertex.pos * pc.scale + pc.translate, 0, 1);
  Out.col       = vertex.col;
  Out.uv        = vertex.uv;
}
