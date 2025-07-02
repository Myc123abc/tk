// TODO: currently, only single glyph for test
//       need to expand multiple glyphs by buffer address to storage vertices and indices

layout(push_constant) uniform PushConstant
{
  vec4 pos; // position of glyph in framebuffer
  vec4 uv;  // coordinate of glyph in font atlas
  // uint font_id; // TODO: expand to multi-font-atlas
  vec2 window_extent;
} pc;