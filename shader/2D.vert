#version 460
#extension GL_EXT_buffer_reference : require

struct Vertex
{
  vec2  position;
};

layout (std430, buffer_reference, buffer_reference_align = 8) readonly buffer Vertices 
{
  Vertex data[];
};

layout(push_constant) uniform PushConstant
{
  mat4     model;
  vec4     color_depth;
  Vertices vertices;
} info;

layout(location = 0) out vec3 out_color;

void main()
{
  Vertex vertex = info.vertices.data[gl_VertexIndex];
  gl_Position   = info.model * vec4(vertex.position, info.color_depth.w, 1.f);
  out_color     = info.color_depth.xyz;
}
