export module tk.renderer.image_handle;

import tk.base;
import tk.object_pool;

export namespace tk::renderer {

struct ImageHandleTag {};

inline constexpr auto Image_Pool_Init_Capacity = uint16{ 64 };
using ImageHandle = ObjectPoolHandle<ImageHandleTag>;

}

export namespace tk::ui {

using ImageHandle = renderer::ImageHandle;

}
