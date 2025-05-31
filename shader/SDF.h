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

#define GetData(idx)  pc.buf.data[idx]
#define GetDataF(idx) uintBitsToFloat(GetData(idx))
#define GetVec2(idx)  vec2(GetDataF(idx), GetDataF(idx + 1))
#define GetVec3(idx)  vec3(GetDataF(idx), GetDataF(idx + 1), GetDataF(idx + 2))
#define GetVec4(idx)  vec4(GetDataF(idx), GetDataF(idx + 1), GetDataF(idx + 2), GetDataF(idx + 3))

#define Line      0
#define Rectangle 1
#define Triangle  2
#define Polygon   3
#define Circle    4

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

float sdTriangle( in vec2 p, in vec2 p0, in vec2 p1, in vec2 p2 )
{
    vec2 e0 = p1-p0, e1 = p2-p1, e2 = p0-p2;
    vec2 v0 = p -p0, v1 = p -p1, v2 = p -p2;
    vec2 pq0 = v0 - e0*clamp( dot(v0,e0)/dot(e0,e0), 0.0, 1.0 );
    vec2 pq1 = v1 - e1*clamp( dot(v1,e1)/dot(e1,e1), 0.0, 1.0 );
    vec2 pq2 = v2 - e2*clamp( dot(v2,e2)/dot(e2,e2), 0.0, 1.0 );
    float s = sign( e0.x*e2.y - e0.y*e2.x );
    vec2 d = min(min(vec2(dot(pq0,pq0), s*(v0.x*e0.y-v0.y*e0.x)),
                     vec2(dot(pq1,pq1), s*(v1.x*e1.y-v1.y*e1.x))),
                     vec2(dot(pq2,pq2), s*(v2.x*e2.y-v2.y*e2.x)));
    return -sqrt(d.x)*sign(d.y);
}

float sdPolygon( in uint offset, in uint n, in vec2 p )
{
    float d = dot(p-GetVec2(offset),p-GetVec2(offset));
    float s = 1.0;
    uint count = n*2;
    for( uint i=0, j=(n-1)*2; i<count; j=i, i+=2 )
    {
        vec2 vj = GetVec2(offset+j);
        vec2 vi = GetVec2(offset+i);
        vec2 e = vj - vi;
        vec2 w =    p - vi;
        vec2 b = w - e*clamp( dot(w,e)/dot(e,e), 0.0, 1.0 );
        d = min( d, dot(b,b) );
        bvec3 c = bvec3(p.y>=vi.y,p.y<vj.y,e.x*w.y>e.y*w.x);
        if( all(c) || all(not(c)) ) s*=-1.0;  
    }
    return s*sqrt(d);
}

float sdCircle( vec2 p, float r )
{
    return length(p) - r;
}