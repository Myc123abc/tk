# Text Processing Pipeline

1. The public text API handles empty text and converts size, tracking, and wrapping
   limits to the internal text coordinate system.

2. Look up the parse-result cache before doing new work. Return the matching ready
   result, a last-ready fallback for the same text and direction, or the matching
   pending result. Continue parsing only when there is no matching cache entry.

3. Resolve fonts at grapheme boundaries. Group adjacent graphemes using the same
   font into font runs.

4. Shape each font run, not each grapheme independently, preserving shaping
   context such as ligatures. Vertical text is further split by shaping
   orientation. Record glyph indices, advances, offsets, orientations, and UTF-8
   byte cluster offsets.

5. Request unavailable glyphs from the glyph store and record which ones the
   result is waiting for. Already available glyphs are reused. Other glyphs may
   come from the disk cache or require bitmap generation.

6. Record parse entries reported as containing missing glyphs. Loading a
   supporting font can invalidate these entries so the text is parsed again.

7. Initialize text metrics according to the text direction.
   - Horizontal: apply tracking, then wrapping immediately.
   - Vertical with glyph metrics available: prepare layout using actual glyph
     bounds, then apply tracking and wrapping.
   - Vertical with glyphs pending: defer layout preparation. When tracking or
     wrapping is requested, save the unmodified advances, extent, and layout
     inputs before applying provisional tracking and wrapping.

8. Cache the new parse result, even if some glyphs are still pending.
   - If ready, finalize it and record it as the last-ready result for this text
     and direction.
   - If pending, register it for completion during updates. Return a last-ready
     fallback for the same text and direction when available; otherwise return
     the pending result itself.

9. During updates, load cached bitmaps or generate new ones, pack them into glyph
   atlases, and schedule their GPU uploads. Once every glyph required by a pending
   result is available:
   - Clear its pending-glyph set.
   - For vertical text, restore saved advances and extent if present, prepare
     layout using actual glyph bounds, and reapply saved tracking and wrapping.
   - Finalize the result and remove it from the generating set.
   - A subsequent parse/cache lookup promotes it to the last-ready result.

## Finalization

`finalize()` measures glyph bitmap bounds at their final positions. When color
or sideways glyphs require bounds adjustment, it combines those bounds with the
logical layout bounds, normalizes the layout origin, and updates the extent,
layout offset, horizontal ascender, and cached bounding rectangle.

Ordinary monochrome upright text usually requires no adjustment in this step.
Finalization does not shape text, generate bitmaps, apply tracking, or wrap lines.

## Public API Results and Rendering

The public text API returns scaled extent and ascender values. If no ready
fallback exists while glyphs are pending, these values are provisional.
Horizontal and vertical layouts never serve as fallbacks for each other.
The bounding-rectangle API returns no rectangle while its selected result is
pending.

Drawing text queues a draw command. During command processing, a pending result
causes the entire text command to be skipped until it is ready; no loading
placeholder or partial text is drawn. Ready results produce glyph vertices using
their layout offsets, glyph positions, and atlas textures.
