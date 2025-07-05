// TODO: currently, only single glyph for test
//       need to expand multiple glyphs by buffer address to storage vertices and indices

#extension GL_EXT_buffer_reference : require

struct Vertex
{
  vec2  pos;
  vec2  uv;
};

layout(std430, buffer_reference) readonly buffer Vertices 
{
  Vertex data[];
};

layout(push_constant) uniform PushConstant
{
  Vertices vertices;
  vec2 window_extent;
  // uint font_id; // TODO: expand to multi-font-atlas
} pc;