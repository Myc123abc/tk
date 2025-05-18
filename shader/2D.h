#extension GL_EXT_buffer_reference : require

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

// TODO: only scale and translate, not relative window coordinate to reduce vkDrawIndexed number
layout(push_constant) uniform PushConstant
{
  Vertices vertices;
  vec2     window_extent;
  vec2     display_pos;
} pc;