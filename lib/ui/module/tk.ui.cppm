export module tk.ui;

export import :animation;
export import :common;
export import :geometry;
export import :image;
export import :input;
export import :line_chart;
export import :path;
export import :shape_operation;
export import :text_layout;
export import :text;
export import :transform;
export import :tween;
export import :widget;
export import :window;
export import tk.ui.color;
export import tk.ui.image_error;
export import tk.ui.window_config;

export namespace tk {

// initialize tk library
void init() noexcept;

// destroy the tk library
void destroy() noexcept;

// process window message and renderer rendering
void update() noexcept;

}
