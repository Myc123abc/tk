#version 460
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec3 out_color;

struct Vertex
{
  vec3 pos;
  float pad0;
  vec3 normal;
  float pad1;
  vec4 color;
};

layout (buffer_reference, std430) readonly buffer VertexBuffer 
{
  Vertex vertices[];
};

layout (push_constant) uniform PushConstant 
{
  mat4         world_matrix;
  VertexBuffer vertex_buffer;
} push_constant;

void main()
{
  Vertex vertex = push_constant.vertex_buffer.vertices[gl_VertexIndex];

  gl_Position = push_constant.world_matrix * vec4(vertex.pos, 1.f);

  out_color = vertex.color.xyz;
}
