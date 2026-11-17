export module tk.ui:common;

export namespace tk::ui {

/**
 * get frame delta time
 * @return delta time(us)
 */
auto delta_time() noexcept -> double;

enum class CornerFlag
{
  top_left     = 1 << 0,
  top_right    = 1 << 1,
  bottom_left  = 1 << 2,
  bottom_right = 1 << 3,

  none   = 1 << 4,
  top    = top_left    | top_right,
  bottom = bottom_left | bottom_right,
  left   = top_left    | bottom_left,
  right  = top_right   | bottom_right,
  all    = top         | bottom,
};

}
