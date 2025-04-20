//
// test tk library
//
// tk use SDL3 callback system to organize program,
// and using internal variables to process graphics engine initialization,
// event handles and other.
//

#include "tk/tk.hpp"
#include "tk/log.hpp"

// TODO:
//1.same place same depth widget cannot

struct Context
{
  tk::ui::Layout* layout;
  tk::ui::Button* button0;
  tk::ui::Button* button1;
};

void tk_init(int argc, char** argv)
{
  auto ctx = new Context();
  tk::init_tk_context("tk", 1000, 1000, ctx);

  ctx->layout = tk::ui::create_layout();
  ctx->button0 = tk::ui::create_button(100, 100, tk::Color::Blue);
  ctx->button1 = tk::ui::create_button(100, 100, tk::Color::Red);

  tk::ui::put(ctx->layout, tk::get_main_window(), 0, 0);
  tk::ui::put(ctx->button0, ctx->layout, 100, 100);
  tk::ui::put(ctx->button1, ctx->layout, 100, 100);
  ctx->button0->set_depth(0.2f);
}

static uint32_t x, y;

void tk_iterate()
{
  auto ctx = (Context*)tk::get_user_data();

  static bool removed = false;
  if (ctx->button0->is_clicked())
  {
    // tk::ui::remove(ctx->button0, ctx->layout);
    x += 50;
    y += 50;
    tk::ui::put(ctx->button0, ctx->layout, x, y);
    // if (removed)
    // {
    //   tk::ui::put(ctx->button, ctx->layout, 0, 0);
    // }
    // else
    // tk::ui::remove(ctx->button, ctx->layout);
  }

  if (ctx->button1->is_clicked())
  {
  }
}

void tk_event(SDL_Event* event)
{
}

void tk_quit()
{
  delete (Context*)tk::get_user_data();
}
