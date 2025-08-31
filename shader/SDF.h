#extension GL_EXT_buffer_reference : require

struct Vertex
{
  vec2 pos;
  vec2 uv;
  uint offset;
  uint glyph_atlases_index;
};

layout(std430, buffer_reference) readonly buffer Vertices 
{
  Vertex data[];
};

// TODO: min ( union )
layout(std430, buffer_reference) readonly buffer ShapeProperties
{
  uint data[];  // shape type  |  color  |  thickness  |  operator  |  values
                //   uint     |   vec4  |     uint    |    uint    |    ...

                //   path  |  color  |  thickness  |  operator |  count  |  partition type  |  values   |   ...
                //  uint  |   vec4  |     uint    |    uint   |   uint  |       uint       |    ...    |   ...

                //  glyph  |  inner_color  |  outer_color  |  outline width
                //  uint  |      vec4     |     vec4      |     float
};

layout(push_constant) uniform PushConstant
{
  Vertices        vertices;
  ShapeProperties shape_properties;
  vec2            window_extent;
} pc;

#define Line             0
#define Rectangle        1
#define Triangle         2
#define Polygon          3
#define Circle           4
#define Bezier           5

#define Path             6
#define Line_Partition   7
#define Bezier_Partition 8
#define Glyph            9

#define Mix              0
#define Min              1

#define HeaderSize 7

#define GetData(idx)  pc.shape_properties.data[idx]
#define GetDataF(idx) uintBitsToFloat(GetData(idx))
#define GetVec2(idx)  vec2(GetDataF(idx), GetDataF(idx + 1))
#define GetVec3(idx)  vec3(GetDataF(idx), GetDataF(idx + 1), GetDataF(idx + 2))
#define GetVec4(idx)  vec4(GetDataF(idx), GetDataF(idx + 1), GetDataF(idx + 2), GetDataF(idx + 3))

#define GetType(x) GetData(x)
#define GetP0(x)   GetVec2(x + HeaderSize)
#define GetP1(x)   GetVec2(x + HeaderSize + 2)
#define GetP2(x)   GetVec2(x + HeaderSize + 4)

#define GetPartitionP0(x) GetVec2(x + 1)
#define GetPartitionP1(x) GetVec2(x + 3)
#define GetPartitionP2(x) GetVec2(x + 5)

#define GetPolygonPointsBeginOffset(x) x + HeaderSize + 1

#define GetFirstValue(x)  GetData(x + HeaderSize)
#define GetThirdValueF(x) GetDataF(x + HeaderSize + 2)

#define GetPathCount(x)            GetData(x + HeaderSize)
#define GetPathBeginOffset(x)      x + HeaderSize + 1
#define GetLinePartitionOffset()   5
#define GetBezierPartitionOffset() 7

#define GetColor(x)     GetVec4(x + 1)
#define GetThickness(x) GetData(x + 5)
#define GetOperator(x)  GetData(x + 6)

#define GetInnerColor(x)        GetVec4(x + 1)
#define GetOuterColor(x)        GetVec4(x + 5)
#define GetOutlineWidht(x)      GetDataF(x + 9)

////////////////////////////////////////////////////////////////////////////////
//                            SDF functions
////////////////////////////////////////////////////////////////////////////////

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

float dot2( vec2 v ) { return dot(v,v); }

float sdBezier( in vec2 pos, in vec2 A, in vec2 B, in vec2 C )
{    
    vec2 a = B - A;
    vec2 b = A - 2.0*B + C;
    vec2 c = a * 2.0;
    vec2 d = A - pos;
    float kk = 1.0/dot(b,b);
    float kx = kk * dot(a,b);
    float ky = kk * (2.0*dot(a,a)+dot(d,b)) / 3.0;
    float kz = kk * dot(d,a);      
    float res = 0.0;
    float p = ky - kx*kx;
    float p3 = p*p*p;
    float q = kx*(2.0*kx*kx-3.0*ky) + kz;
    float h = q*q + 4.0*p3;
    if( h >= 0.0) 
    { 
        h = sqrt(h);
        vec2 x = (vec2(h,-h)-q)/2.0;
        vec2 uv = sign(x)*pow(abs(x), vec2(1.0/3.0));
        float t = clamp( uv.x+uv.y-kx, 0.0, 1.0 );
        res = dot2(d + (c + b*t)*t);
    }
    else
    {
        float z = sqrt(-p);
        float v = acos( q/(p*z*2.0) ) / 3.0;
        float m = cos(v);
        float n = sin(v)*1.732050808;
        vec3  t = clamp(vec3(m+m,-n-m,n-m)*z-kx,0.0,1.0);
        res = min( dot2(d+(c+b*t.x)*t.x),
                   dot2(d+(c+b*t.y)*t.y) );
        // the third root cannot be the closest
        // res = min(res,dot2(d+(c+b*t.z)*t.z));
    }
    return sqrt( res );
}

////////////////////////////////////////////////////////////////////////////////
//                      line and bezier with partition
////////////////////////////////////////////////////////////////////////////////

const float SQRT3 = 1.732050807568877;

// Clamp a value to [0, 1]
float saturate(in float a) {
    return clamp(a, 0.0, 1.0);
}
vec3 saturate(in vec3 a) {
    return clamp(a, 0.0, 1.0);
}

// Cross-product of two 2D vectors
float cross2(in vec2 a, in vec2 b) {
    return a.x*b.y - a.y*b.x;
}

// Like the SDF for a line but partitioning space into positive and negative
float sdf_line_partition(in vec2 p, in vec2 a, in vec2 b) {
    vec2 ba = b - a;
    vec2 pa = p - a;
    float h = saturate(dot(pa, ba) / dot(ba, ba));
    vec2 k = pa - h * ba;
    vec2 n = vec2(ba.y, -ba.x);
    return (dot(k,n) >= 0.0) ? length(k) : -length(k);
}

// Signed distance to a quadratic Bézier curve
// Mostly identical to https://www.shadertoy.com/view/MlKcDD
// with some additions to combat degenerate cases.
float sdf_bezier_partition(in vec2 pos, in vec2 A, in vec2 B, in vec2 C) {
    const float EPSILON = 1e-3;
    const float ONE_THIRD = 1.0 / 3.0;

    // Handle cases where points coincide
    bool abEqual = all(equal(A, B));
    bool bcEqual = all(equal(B, C));
    bool acEqual = all(equal(A, C));
    
    if (abEqual && bcEqual) {
        return distance(pos, A);
    } else if (abEqual || acEqual) {
        return sdf_line_partition(pos, B, C);
    } else if (bcEqual) {
        return sdf_line_partition(pos, A, C);
    }
    
    // Handle colinear points
    if (abs(dot(normalize(B - A), normalize(C - B)) - 1.0) < EPSILON) {
        return sdf_line_partition(pos, A, C);
    }
    
    vec2 a = B - A;
    vec2 b = A - 2.0*B + C;
    vec2 c = a * 2.0;
    vec2 d = A - pos;

    float kk = 1.0 / dot(b,b);
    float kx = kk * dot(a,b);
    float ky = kk * (2.0*dot(a,a)+dot(d,b)) * ONE_THIRD;
    float kz = kk * dot(d,a);

    float res = 0.0;
    float sgn = 0.0;

    float p = ky - kx*kx;
    float p3 = p*p*p;
    float q = kx*(2.0*kx*kx - 3.0*ky) + kz;
    float h = q*q + 4.0*p3;

    if (h >= 0.0) {
        // One root
        h = sqrt(h);
        vec2 x = 0.5 * (vec2(h, -h) - q);
        vec2 uv = sign(x) * pow(abs(x), vec2(ONE_THIRD));
        float t = saturate(uv.x + uv.y - kx) + EPSILON;
        vec2 q = d + (c + b*t) * t;
        res = dot(q, q);
        sgn = cross2(c + 2.0*b*t, q);
    } else {
        // Three roots
        float z = sqrt(-p);
        float v = acos(q/(p*z*2.0)) * ONE_THIRD;
        float m = cos(v);
        float n = sin(v) * SQRT3;
        vec3 t = saturate(vec3(m+m,-n-m,n-m)*z-kx) + EPSILON;
        vec2 qx = d + (c+b*t.x)*t.x;
        float dx = dot(qx, qx);
        float sx = cross2(c+2.0*b*t.x, qx);
        vec2 qy = d + (c+b*t.y)*t.y;
        float dy = dot(qy, qy);
        float sy = cross2(c+2.0*b*t.y, qy);
        res = (dx < dy) ? dx : dy;
        sgn = (dx < dy) ? sx : sy;
    }
    
    return sign(sgn) * sqrt(res);
}