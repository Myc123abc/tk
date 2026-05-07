float sdTriangle(float2 p, float2 p0, float2 p1, float2 p2)
{
  float2 e0 = p1-p0, e1 = p2-p1, e2 = p0-p2;
  float2 v0 = p -p0, v1 = p -p1, v2 = p -p2;
  float2 pq0 = v0 - e0*clamp( dot(v0,e0)/dot(e0,e0), 0.0, 1.0 );
  float2 pq1 = v1 - e1*clamp( dot(v1,e1)/dot(e1,e1), 0.0, 1.0 );
  float2 pq2 = v2 - e2*clamp( dot(v2,e2)/dot(e2,e2), 0.0, 1.0 );
  float s = sign( e0.x*e2.y - e0.y*e2.x );
  float2 d = min(min(float2(dot(pq0,pq0), s*(v0.x*e0.y-v0.y*e0.x)),
                     float2(dot(pq1,pq1), s*(v1.x*e1.y-v1.y*e1.x))),
                     float2(dot(pq2,pq2), s*(v2.x*e2.y-v2.y*e2.x)));
  return -sqrt(d.x)*sign(d.y);
}

float sdBox(float2 p, float2 b)
{
  float2 d = abs(p)-b;
  return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

float sdCircle(float2 p, float r)
{
  return length(p) - r;
}

float sdSegment(in float2 p, in float2 a, in float2 b)
{
  float2 pa = p - a, ba = b - a;
  float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
  return length(pa - ba * h);
}

////////////////////////////////////////////////////////////////////////////////
//                                  arc
////////////////////////////////////////////////////////////////////////////////

float sdArc( in float2 p, in float2 sc, in float ra, float rb )
{
    // sc is the sin/cos of the arc's aperture
    p.x = abs(p.x);
    return ((sc.y*p.x>sc.x*p.y) ? length(p-sc*ra) : 
                                  abs(length(p)-ra)) - rb;
}

static const float PI  = 3.14159265358979323846;
static const float TAU = 6.28318530717958647692;

float normalize_angle(float a)
{
    a = fmod(a, TAU);
    if (a < 0.0) a += TAU;
    return a;
}

float angle_delta_ccw(float from, float to)
{
    float d = normalize_angle(to) - normalize_angle(from);
    if (d < 0.0) d += TAU;
    return d;
}

float2 rotate2d(float2 p, float a)
{
    float s = sin(a);
    float c = cos(a);
    return float2(c * p.x - s * p.y, s * p.x + c * p.y);
}

float sdArcAngles(float2 p, float2 center, float radius, float beg, float end, bool ccw, float halfThickness)
{
    beg = normalize_angle(beg);
    end = normalize_angle(end);

    float sweep = ccw ? angle_delta_ccw(beg, end) : angle_delta_ccw(end, beg);
    float mid   = ccw ? normalize_angle(beg + sweep * 0.5) : normalize_angle(beg - sweep * 0.5);
    float halfA = sweep * 0.5;

    float2 q = p - center;
    q = rotate2d(q, -mid);

    float2 sc = float2(sin(halfA), cos(halfA));
    return sdArc(q, sc, radius, halfThickness);
}

////////////////////////////////////////////////////////////////////////////////
//                                  bezier
////////////////////////////////////////////////////////////////////////////////

float dot2(float2 v) { return dot(v, v); }

float sdBezier(in float2 pos, in float2 A, in float2 B, in float2 C)
{
    float2 a = B - A;
    float2 b = A - 2.0 * B + C;
    float2 c = a * 2.0;
    float2 d = A - pos;
    float kk = 1.0 / dot(b, b);
    float kx = kk * dot(a, b);
    float ky = kk * (2.0 * dot(a, a) + dot(d, b)) / 3.0;
    float kz = kk * dot(d, a);
    float res = 0.0;
    float p = ky - kx * kx;
    float p3 = p * p * p;
    float q = kx * (2.0 * kx * kx - 3.0 * ky) + kz;
    float h = q * q + 4.0 * p3;
    if (h >= 0.0)
    {
        h = sqrt(h);
        float2 x = (float2(h, -h) - q) / 2.0;
        float2 uv = sign(x) * pow(abs(x), float2(1.0 / 3.0, 1.0 / 3.0));
        float t = clamp(uv.x + uv.y - kx, 0.0, 1.0);
        res = dot2(d + (c + b * t) * t);
    }
    else
    {
        float z = sqrt(-p);
        float v = acos(q / (p * z * 2.0)) / 3.0;
        float m = cos(v);
        float n = sin(v) * 1.732050808;
        float3 t = clamp(float3(m + m, -n - m, n - m) * z - kx, 0.0, 1.0);
        res = min(dot2(d + (c + b * t.x) * t.x),
                  dot2(d + (c + b * t.y) * t.y));
        // the third root cannot be the closest
        // res = min(res,dot2(d+(c+b*t.z)*t.z));
    }
    return sqrt(res);
}

////////////////////////////////////////////////////////////////////////////////
//                      line and bezier with partition
////////////////////////////////////////////////////////////////////////////////

#define SQRT3 1.732050807568877

// Clamp a value to [0, 1]
float saturate(in float a) {
    return clamp(a, 0.0, 1.0);
}
float3 saturate(in float3 a) {
    return clamp(a, 0.0, 1.0);
}

// Cross-product of two 2D vectors
float cross2(in float2 a, in float2 b) {
    return a.x*b.y - a.y*b.x;
}

// Like the SDF for a line but partitioning space into positive and negative
float sdf_line_partition(in float2 p, in float2 a, in float2 b) {
    float2 ba = b - a;
    float2 pa = p - a;
    float h = saturate(dot(pa, ba) / dot(ba, ba));
    float2 k = pa - h * ba;
    float2 n = float2(ba.y, -ba.x);
    return (dot(k,n) >= 0.0) ? length(k) : -length(k);
}

// Signed distance to a quadratic Bézier curve
// Mostly identical to https://www.shadertoy.com/view/MlKcDD
// with some additions to combat degenerate cases.
float sdf_bezier_partition(in float2 pos, in float2 A, in float2 B, in float2 C) {
    const float EPSILON = 1e-3;
    const float ONE_THIRD = 1.0 / 3.0;

    // Handle cases where points coincide
    bool abEqual = all(A == B);
    bool bcEqual = all(B == C);
    bool acEqual = all(A == C);
    
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
    
    float2 a = B - A;
    float2 b = A - 2.0*B + C;
    float2 c = a * 2.0;
    float2 d = A - pos;

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
        float2 x = 0.5 * (float2(h, -h) - q);
        float2 uv = sign(x) * pow(abs(x), float2(ONE_THIRD, ONE_THIRD));
        float t = saturate(uv.x + uv.y - kx) + EPSILON;
        float2 q = d + (c + b*t) * t;
        res = dot(q, q);
        sgn = cross2(c + 2.0*b*t, q);
    } else {
        // Three roots
        float z = sqrt(-p);
        float v = acos(q/(p*z*2.0)) * ONE_THIRD;
        float m = cos(v);
        float n = sin(v) * SQRT3;
        float3 t = saturate(float3(m+m,-n-m,n-m)*z-kx) + EPSILON;
        float2 qx = d + (c+b*t.x)*t.x;
        float dx = dot(qx, qx);
        float sx = cross2(c+2.0*b*t.x, qx);
        float2 qy = d + (c+b*t.y)*t.y;
        float dy = dot(qy, qy);
        float sy = cross2(c+2.0*b*t.y, qy);
        res = (dx < dy) ? dx : dy;
        sgn = (dx < dy) ? sx : sy;
    }
    
    return sign(sgn) * sqrt(res);
}
