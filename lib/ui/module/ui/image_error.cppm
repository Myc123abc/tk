module;

#include <string_view>

export module tk.ui.image_error;

import tk.variant;

export namespace tk::ui {

namespace ImageLoadError {
struct unexist {};
struct loading {};
struct decode_failed { std::string_view msg; };

using Type = Variant<unexist, loading, decode_failed>;
}

using ImageLoadErrorType = ImageLoadError::Type;

}
