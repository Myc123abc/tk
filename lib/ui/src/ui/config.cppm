export module tk.ui.config;

export namespace tk::ui {

constexpr auto Title_Bar_Height             = 35;
constexpr auto Title_Bar_Button_Width       = 46;
constexpr auto Title_Bar_Button_Icon_Width  = 10;
constexpr auto Title_Bar_Button_Icon_Height = 10;

// Limit zoom before float geometry loses pixel precision and destabilizes tessellation.
constexpr auto Viewport_Max_Scale = 8192.f;

}
