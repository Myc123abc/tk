#extension GL_EXT_buffer_reference : require

/*
  buffer:      points | shape_infos

  shape_infos: offset | num | color
*/

layout(std430, buffer_reference) readonly buffer Buffer
{
  uint data[];
};

layout(push_constant) uniform PushConstant
{
  Buffer buf;
  uint   offset; // offset of shape infos
  uint   num;    // number of shape infos
  vec2   window_extent;
} pc;

float sdSegment( in vec2 p, in vec2 a, in vec2 b )
{
    vec2 pa = p-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length( pa - ba*h );
}

float sdBox( in vec2 p, in vec2 b )
{
    vec2 d = abs(p)-b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

//float sdHalfPlane( in vec2 p, in vec2 p0, in vec2 n)
//{
//  return dot(p - p0, n);
//}