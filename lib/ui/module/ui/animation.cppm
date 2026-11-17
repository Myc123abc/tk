export module tk.ui:animation;

import :tween;
import tk.ui.color;
import tk.string_id;

export namespace tk::ui {

/**
 * get lerp color
 * @param x begin of color
 * @param y end of color
 * @param v lerp value
 * @return lerp color
 */
auto lerp(Color x, Color y, float v) noexcept -> float4;

/**
 * get lerp value of two points
 * @param x
 * @param y
 * @param v
 * @return lerp value
 */
auto lerp(float2 x, float2 y, float v) noexcept -> float2;

/**
 * get a lerp value in ping pong
 * @param name unique global name
 * @param b drive boolean value
 * @param forward_dur duration (us)
 * @param reverse_dur duration (us)
 * @param ease ease funcation for tween
 * @return lerp value (0.0 ~ 1.0)
 */
auto ping_pong(StringID name, bool b, double forward_dur, double reverse_dur, Tween::Ease ease = Tween::linear) noexcept -> double;
inline auto ping_pong(StringID name, bool b, double duration, Tween::Ease ease = Tween::linear) noexcept { return ping_pong(name, b, duration, duration, ease); }

/**
 * reset tween
 * @param name name of tween
 */
void reset_tween(StringID name) noexcept;

}
