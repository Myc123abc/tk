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
  add_arc,
  add_bezier_quad,
  add_image,

  add_path,
  add_path_line,
  add_path_arc,
  add_path_bezier_quad,
};

enum : int
{
  op_none          = 0b000,
  op_uni           = 0b001,
  op_discard_shape = 0b010,
  op_discard       = 0b100
};

struct DrawCmd
{
  int    type;
  float4 color;
  float  thickness;
  int    op;
  uint   uni_cnt;
  uint   discard_cnt;
  float2 p0;
  float2 p1;
  float2 p2;
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
StructuredBuffer<DrawCmd> path_cmds : register(t0, space3);
Texture2D                 images[]  : register(t1);
