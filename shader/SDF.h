#extension GL_EXT_buffer_reference : require

struct Vertex
{
  vec2 pos;
  vec2 uv;
  uint color;
  uint padding; // replace to thickness can lerp?
};

layout(std430, buffer_reference) readonly buffer Vertices 
{
  Vertex data[];
};

layout(std430, buffer_reference) readonly buffer ShapeProperties
{
  uint data[];
};

layout(push_constant) uniform PushConstant
{
  Vertices        vertices;
  ShapeProperties shape_properties;
  vec2            window_extent;
} pc;

#define GetData(idx)  pc.shape_properties.data[idx]
#define GetDataF(idx) uintBitsToFloat(GetData(idx))
#define GetVec2(idx)  vec2(GetDataF(idx), GetDataF(idx + 1))
#define GetVec3(idx)  vec3(GetDataF(idx), GetDataF(idx + 1), GetDataF(idx + 2))
#define GetVec4(idx)  vec4(GetDataF(idx), GetDataF(idx + 1), GetDataF(idx + 2), GetDataF(idx + 3))

#define Rectangle 0

////////////////////////////////////////////////////////////////////////////////
//                            SDF functions
////////////////////////////////////////////////////////////////////////////////

float sdBox( in vec2 p, in vec2 b )
{
    vec2 d = abs(p)-b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}