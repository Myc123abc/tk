export module tk.ui:input;

export import tk.ui.input_types;
import tk.flag;

export namespace tk::ui {

struct GetKeyResult
{
  Flag<KeyState> state{};

  constexpr operator bool() const noexcept
  {
    return state == KeyState::down || state == KeyState::press;
  }

  auto has_down() const noexcept { return state.contains(KeyState::down); }
  auto is_down()  const noexcept { return state == KeyState::down;        }

  auto is_uppercase() const noexcept -> bool;
  auto is_lowercase() const noexcept -> bool { return !is_uppercase(); }
};

/**
 * Get key state
 * @param key
 * @return result of key state
 */
auto get_key(Key key) noexcept -> GetKeyResult;

/**
 * Get the vertical mouse-wheel delta accumulated for this frame.
 * One wheel detent is 1.0; positive values scroll up and negative values scroll down.
 */
auto get_mouse_wheel_delta() noexcept -> float;

}
