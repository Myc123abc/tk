#pragma once

#include "tk/ui/ui.hpp"
#include "tk/log.hpp"

using namespace tk;

inline auto wnd_cfg = ui::WindowConfig
{
  .display_title_bar             = true,
	.display_window_shadow         = true,
  .wireframe_color               = 0x7160e8ff,
  .display_wireframe_only_active = true,
  .backdrop                      = ui::Backdrop::Default_Blur(),
};

inline auto load_image(std::string_view path) noexcept
{
  if (auto res = ui::load_image(path); !res)
    res.error().visit(
      [&](ui::ImageLoadError::loading) { info("loading {}", path); },
      [&](ui::ImageLoadError::unexist) { warn("unexist image {}", path); },
      [&](ui::ImageLoadError::decode_failed const& err) { warn("decode failed of image {} : {}", path, err.msg); }
    );
}

inline auto image(std::string_view path, float2 left_top, float2 right_bottom, uint8 alpha = 0xff) noexcept -> std::expected<void, ui::ImageLoadErrorType>
{
  return ui::image(path, left_top, right_bottom, alpha).or_else([&](ui::ImageLoadErrorType err)
  {
    err.visit(
      [&](ui::ImageLoadError::loading) { info("loading {}", path); },
      [&](ui::ImageLoadError::unexist) { warn("unexist image {}", path); },
      [&](ui::ImageLoadError::decode_failed const& err) { warn("decode failed of image {} : {}", path, err.msg); }
    );
    return std::expected<void, ui::ImageLoadErrorType>{ std::unexpected(err) };
  });
}
