module;

#include <expected>
#include <optional>
#include <string_view>

module tk.ui;

import :image;
import :ui.util;
import tk.base;
import tk.renderer;
import tk.ui.context;

namespace tk::ui {

auto image_extent(std::string_view path) noexcept -> float2
{
  return renderer::g_img_mgr.extent(path);
}

auto image(std::string_view path, float2 left_top, float2 right_bottom, uint8 alpha, std::optional<ImageConfig> cfg) noexcept -> std::expected<void, ImageLoadErrorType>
{
  if (!alpha || left_top.x == right_bottom.x || left_top.y == right_bottom.y) return {};
  g_ctx.register_item({ left_top, right_bottom });
  detail::adjust_pos(left_top, right_bottom);
  if (cfg && cfg->rounding_cfg) detail::adjust_scale(cfg->rounding_cfg->radius);
  return g_ctx.image(path, left_top, right_bottom, alpha, cfg);
}

auto load_image(std::string_view path) noexcept -> std::expected<void, ImageLoadErrorType>
{
  return renderer::g_img_mgr.try_load(path, {}, {}).transform([](auto&&) {});
}

}
