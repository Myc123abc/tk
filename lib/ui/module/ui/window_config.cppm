module;

#include <optional>

export module tk.ui.window_config;

import tk.ui.color;
import tk.base;

export namespace tk::ui {

struct WindowConfig
{
  bool                 display_title_bar{};
  bool                 display_window_shadow{};
  std::optional<Color> wireframe_color{};
  bool                 display_wireframe_only_active{};
  bool                 no_resize{};
  bool                 no_move{};

  struct Backdrop
  {
    enum class Style
    {
      none,
      blur,
      acrylic,
    } style;

    union
    {
      float blur_radius{};
      struct Acrylic
      {
        float opacity{};
        float blur{};
        float4 tint_color{};
        float4 luminosity_color{};
      } acrylic;
    };

    static auto Default_Blur() noexcept -> Backdrop
    {
      return
      {
        .style       = Style::blur,
        .blur_radius = 4.f,
      };
    }

    static auto Default_Acrylic() noexcept -> Backdrop
    {
      return
      {
        .style   = Style::acrylic,
        .acrylic =
        {
          .opacity          = .02f,
          .blur             = 30.f,
          .tint_color       = { .125f, .125f, .125f, .4f },
          .luminosity_color = { .125f, .125f, .125f, .8f },
        },
      };
    }

    auto operator!=(Backdrop const& b) const noexcept
    {
      if (style != b.style) return true;
      if (style == Style::blur)
        return blur_radius != b.blur_radius;
      else if (style == Style::acrylic)
        return acrylic.opacity          != b.acrylic.opacity    ||
               acrylic.blur             != b.acrylic.blur       ||
               acrylic.tint_color       != b.acrylic.tint_color ||
               acrylic.luminosity_color != b.acrylic.luminosity_color;
      return false;
    }

  } backdrop;
};

using Backdrop      = WindowConfig::Backdrop;
using BackdropStyle = WindowConfig::Backdrop::Style;

}
