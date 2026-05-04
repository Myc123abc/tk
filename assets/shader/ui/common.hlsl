struct Constants
{
  uint2  render_target_extent;
  uint2  window_extent;

  float2 window_pos;
  float  shadow_thickness;
  float  shadow_radius;

  float3 shadow_color;
  float  shadow_softness;

  float4 wireframe_color;

  uint2  tile_size;
  uint2  tile_count;
  
  uint   draw_wireframe;
};

enum : int
{
  add_rect,
  add_triangle,
  add_circle,
  add_line,
  add_bezier_quad,
  add_bezier_cubic,
  add_image,
};

struct DrawCmd
{
  int    type;
  float4 color;
  float  thickness;
  float2 p0;
  float2 p1;
  float2 p2;
  float2 p3;
};

struct Tile
{
  uint beg;
  uint count;
};

ConstantBuffer<Constants> constants : register(b0);
SamplerState              g_sampler : register(s0);
StructuredBuffer<DrawCmd> cmds      : register(t0);
StructuredBuffer<uint>    cmd_idxs  : register(t0, space1);
StructuredBuffer<Tile>    tiles     : register(t0, space2);
Texture2D                 images[]  : register(t1);
