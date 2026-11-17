export module tk.ui.text.handle;

import tk.object_pool;

export namespace tk::ui {

struct TextParseResultHandleTag {};
using TextParseResultHandle = ObjectPoolHandle<TextParseResultHandleTag>;

}
