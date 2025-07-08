#extension GL_EXT_buffer_reference : require

struct Vertex
{
  vec2 pos;
  vec2 uv;
  uint color;
};

layout(std430, buffer_reference) readonly buffer Vertices 
{
  Vertex data[];
};

layout(push_constant) uniform PushConstant
{
  Vertices vertices;
  vec2     window_extent;
} pc;