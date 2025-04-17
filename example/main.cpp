//
// test tk library
//
// tk use SDL3 callback system to organize program,
// and using internal variables to process graphics engine initialization,
// event handles and other.
//

#include "tk/tk.hpp"
#include "tk/log.hpp"

struct Context
{
  tk::ui::Layout* layout;
  tk::ui::Button  button;
};

void tk_init(int argc, char** argv)
{
  auto ctx = new Context();
  tk::init_tk_context("tk", 1000, 1000, ctx);

  ctx->layout = tk::ui::create_layout();
  ctx->button = tk::ui::create_button(100, 100);

  // TODO: try dynamic change
  tk::ui::put(ctx->layout, tk::get_main_window(), 0, 0);
  tk::ui::put(&ctx->button, ctx->layout, 50, 50);
}

void tk_iterate()
{
  auto ctx = (Context*)tk::get_user_data();
  if (ctx->button.is_clicked())
  {
    tk::log::info("button is clicked!");
  }
}

void tk_event(SDL_Event* event)
{
}

void tk_quit()
{
  delete (Context*)tk::get_user_data();
}
