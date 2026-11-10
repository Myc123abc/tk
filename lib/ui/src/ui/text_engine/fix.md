5. Prevent emoji and letters from touching this resolve is right way for mostly text engine doing?
10:17 PM








Not exactly. The current solution works as a defensive fix, but it is not how most general-purpose text engines normally handle spacing.

What text engines usually do
Most engines:

Shape text with HarfBuzz.
Trust the font’s glyph/cluster advances.
Position the next cluster using those advances.
Use ink bounds only for painting, clipping, selection, and reported bounds.
Allow limited glyph overhang; they normally do not perform pixel-level collision detection between every pair of glyphs.
Extra space is usually controlled through letter-spacing/tracking rather than automatically added after emoji.

Why emoji needs special handling here
Segoe UI Emoji does not provide useful vertical metrics for every case. Its bitmap ink can extend beyond HarfBuzz’s synthesized vertical advance. Therefore, trusting only the advance allows the following rotated Latin glyph to touch the emoji.

So repairing an under-sized emoji advance is reasonable. However, the current implementation uses two mechanisms:

color_glyph_vertical_advance always adds 1/16 em.
ParseResult::finalize checks actual neighboring ink and adds more spacing if necessary.
This is effective, but it is somewhat heuristic.

Better general-engine design
A cleaner architecture would calculate metrics per grapheme cluster:


For upright color emoji, synthesize only the minimum required inline advance:


Important differences:

Do not always add a gap to an already-correct shaped advance.
Repair only advances that fail to contain the visible emoji ink.
Apply the repaired advance before line wrapping.
Then limit_len and column wrapping use the final spacing directly.
Finalization should measure extents, not modify advances after wrapping.
Optional extra spacing should be exposed as tracking/letter spacing rather than hardcoded into Unicode orientation data.
Recommendation
The current fix is acceptable for this engine and addresses the screenshots correctly. But for a more general text engine, I would refactor it so:

Visible emoji bounds produce a minimum cluster advance.
That advance is calculated before limit_length.
Wrapping uses the repaired advance.
The post-wrap collision/compensation pass is removed.
The 1/16 em gap becomes configurable tracking or a small synthetic trailing bearing.
Also, color_glyph_vertical_advance conceptually does not belong in vertical_orientation.hpp; that file should contain only Unicode orientation data and lookup logic. Emoji advance synthesis belongs in the layout/finalization code.

