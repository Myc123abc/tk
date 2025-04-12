#version 460
#extension GL_EXT_buffer_reference : require

struct Vertex
{
  vec2 position;
  vec3 color;
};

layout (std430, buffer_reference) readonly buffer Vertices 
{
  Vertex data[];
};

layout(push_constant) uniform PushConstant
{
  mat4         model;
  Vertices     vertices;
} info;

layout(location = 0) out vec3 out_color;

void main()
{
  Vertex vertex = info.vertices.data[gl_VertexIndex];
  gl_Position   = info.model * vec4(vertex.position, 0.f, 1.f);
  out_color     = vertex.color;
}
