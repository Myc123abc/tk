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

float sdArc(float2 p, float2 c, float2 p1, float2 p2)
{
    float2 v1 = p1 - c;
    float2 v2 = p2 - c;
    // The parameters are over-determined by one degree of freedom.
    // If p1 and p2 are not on the same distance from c, the arc doesn't
    // actually end in p2, but the end cap is still centered there.
    // Uncomment this line if needed to adjust the distance from p2 to c.
    // v2 = normalize(v2)*length(v1);
    float2 v = p - c;

    // The signs of w.x, w.y are used to determine if we're in the gap
    float2 w = float2(dot(v, -float2(-v1.y, v1.x)), dot(v, float2(-v2.y, v2.x)));
    bool longarc = (dot(v1, float2(-v2.y, v2.x)) < 0.0); // Arc angle > pi
    // Tweak by iq: "fake" OR/AND of booleans by max/min of floats
    float ingap = longarc ? max(w.x,w.y) : min(w.x,w.y);
    return (ingap > 0.0) ? min(length(p1-p), length(p2-p)) : abs(length(v) - length(v1));
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

float2 bezier_quad_point(in float2 A, in float2 B, in float2 C, in float t)
{
    float omt = 1.0 - t;
    return omt * omt * A + 2.0 * omt * t * B + t * t * C;
}

float2 bezier_quad_tangent(in float2 A, in float2 B, in float2 C, in float t)
{
    return 2.0 * (lerp(B - A, C - B, t));
}

int path_line_winding(in float2 pos, in float2 a, in float2 b)
{
    const float EPSILON = 1e-5;
    float dy = b.y - a.y;
    if (abs(dy) <= EPSILON)
        return 0;

    bool crosses = (a.y <= pos.y && b.y > pos.y) || (a.y > pos.y && b.y <= pos.y);
    if (!crosses)
        return 0;

    float t = (pos.y - a.y) / dy;
    float x = lerp(a.x, b.x, t);
    if (x <= pos.x)
        return 0;

    return dy > 0.0 ? 1 : -1;
}

int path_bezier_winding(in float2 pos, in float2 A, in float2 B, in float2 C)
{
    const float EPSILON = 1e-5;

    if ((all(abs(A - B) <= EPSILON) && all(abs(B - C) <= EPSILON)) ||
        (all(abs(A - B) <= EPSILON) || all(abs(B - C) <= EPSILON) || all(abs(A - C) <= EPSILON)))
    {
        return path_line_winding(pos, A, C);
    }

    float qa = A.y - 2.0 * B.y + C.y;
    float qb = 2.0 * (B.y - A.y);
    float qc = A.y - pos.y;
    int winding = 0;

    if (abs(qa) <= EPSILON)
    {
        if (abs(qb) <= EPSILON)
            return 0;

        float t = -qc / qb;
        if (t < 0.0 || t >= 1.0)
            return 0;

        float2 curve_pos = bezier_quad_point(A, B, C, t);
        if (curve_pos.x <= pos.x)
            return 0;

        float dy = bezier_quad_tangent(A, B, C, t).y;
        if (abs(dy) <= EPSILON)
            return 0;

        return dy > 0.0 ? 1 : -1;
    }

    float discriminant = qb * qb - 4.0 * qa * qc;
    if (discriminant < 0.0)
        return 0;

    float root = sqrt(max(discriminant, 0.0));
    float denom = 0.5 / qa;
    float2 ts = float2((-qb - root) * denom, (-qb + root) * denom);

    [unroll]
    for (int i = 0; i < 2; ++i)
    {
        float t = ts[i];
        if (t < 0.0 || t >= 1.0)
            continue;

        float2 curve_pos = bezier_quad_point(A, B, C, t);
        if (curve_pos.x <= pos.x)
            continue;

        float dy = bezier_quad_tangent(A, B, C, t).y;
        if (abs(dy) <= EPSILON)
            continue;

        winding += dy > 0.0 ? 1 : -1;
    }

    return winding;
}

int path_arc_winding(in float2 pos, in float2 center, in float2 p0, in float2 p1)
{
    const float EPSILON = 1e-5;
    float radius = length(p0 - center);
    if (radius <= EPSILON)
        return 0;

    float2 v1 = p0 - center;
    float2 v2 = p1 - center;
    float dy = pos.y - center.y;
    float rr = radius * radius;
    float yy = dy * dy;
    if (yy > rr)
        return 0;

    float dx = sqrt(max(rr - yy, 0.0));
    float2 xs = float2(center.x + dx, center.x - dx);
    bool longarc = dot(v1, float2(-v2.y, v2.x)) < 0.0;
    int winding = 0;

    [unroll]
    for (int i = 0; i < 2; ++i)
    {
        float x = xs[i];
        if (x <= pos.x + EPSILON)
            continue;

        float2 v = float2(x, pos.y) - center;
        float2 w = float2(dot(v, float2(v1.y, -v1.x)), dot(v, float2(-v2.y, v2.x)));
        float ingap = longarc ? max(w.x, w.y) : min(w.x, w.y);
        if (ingap > 0.0)
            continue;

        float tangent_y = x - center.x;
        if (abs(tangent_y) <= EPSILON)
            continue;

        winding += tangent_y > 0.0 ? 1 : -1;
    }

    return winding;
}
