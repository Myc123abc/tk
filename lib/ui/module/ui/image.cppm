module;

#include <string_view>
#include <optional>
#include <expected>

export module tk.ui:image;

import :common;
import tk.ui.image_error;
import tk.base;
import tk.variant;
import tk.flag;

export namespace tk::ui {

/**
 * get extent of image
 * @param path
 * @return width and height of image, if not exist, return (0, 0)
 */
auto image_extent(std::string_view path) noexcept -> float2;

struct ImageConfig
{
  struct Blur
  {
    float sigma{};
    uint  cnt{};
  };
  struct Rounding
  {
    float            radius{};
    Flag<CornerFlag> flags{};
  };
  Variant<Blur>           blur_cfg;
  std::optional<Rounding> rounding_cfg;

  ImageConfig(Blur blur, float radius = {}, Flag<CornerFlag> flags = {}) noexcept
    : blur_cfg(blur), rounding_cfg({ radius, flags }) {}
};

/**
 * display image
 * @param path
 * @param left_top
 * @param right_bottom
 * @param alpha
 * @param cfg can use for blur image
 * @return false if image is unexist, or loading, or load failed
 */
auto image(std::string_view path, float2 left_top, float2 right_bottom, uint8 alpha = 0xff, std::optional<ImageConfig> cfg = {}) noexcept -> std::expected<void, ImageLoadErrorType>;

/**
 * load image
 * @param path
 * @return false if image is unexist, or loading, or load failed
 */
auto load_image(std::string_view path) noexcept -> std::expected<void, ImageLoadErrorType>;

}
