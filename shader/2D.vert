#version 460
#extension GL_EXT_buffer_reference : require

struct Vertex
{
  vec2  position;
  float pad0[2];
  vec3  color;
  float pad1;
};

layout (std430, buffer_reference, buffer_reference_align = 32) readonly buffer Vertices 
{
  Vertex data[];
};

layout(push_constant) uniform PushConstant
{
  mat4     model;
  Vertices vertices;
  float    depth;
} info;

layout(location = 0) out vec3 out_color;

void main()
{
  Vertex vertex = info.vertices.data[gl_VertexIndex];
  gl_Position   = info.model * vec4(vertex.position, info.depth, 1.f);
  out_color     = vertex.color;
}
