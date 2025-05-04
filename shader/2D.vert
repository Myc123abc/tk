#version 460
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

layout(push_constant) uniform PushConstant
{
  Vertices vertices;
  vec2     window_extent;
  vec2     display_pos;
} pc;

layout(location = 0) out struct
{
  vec4 col;
  vec2 uv;
} Out;

void main()
{
  // get vertex
  Vertex vertex  = pc.vertices.data[gl_VertexIndex];
  
  // set vertex position
  vec2 scale     = 2 / pc.window_extent;
  vec2 translate = vec2(-1, -1);
  gl_Position    = vec4((vertex.pos + pc.display_pos) * scale + translate, 0, 1);

  // get color
  float r = float((vertex.col >> 24) & 0xFF) / 255;
  float g = float((vertex.col >> 16) & 0xFF) / 255;
  float b = float((vertex.col >> 8 ) & 0xFF) / 255;
  float a = float((vertex.col      ) & 0xFF) / 255;
  Out.col = vec4(r, g, b, a);

  // set uv
  Out.uv  = vertex.uv;
}
