//
// test tk library
//
// tk use SDL3 callback system to organize program,
// and using internal variables to process graphics engine initialization,
// event handles and other.
//

#include "tk/tk.hpp"
#include "tk/log.hpp"

// FIX: cannot remove
//      cannot change buttons in same place
//      cannot move button not have old button when clicked
//      cannot right mouse button trige
// TODO:move button

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
  // tk::ui::put(ctx->button1, ctx->layout, 100, 100);
}

static uint32_t x, y;

void tk_iterate()
{
  auto ctx = (Context*)tk::get_user_data();

  bool removed = false;
  if (ctx->button0->is_clicked())
  {
    tk::ui::remove(ctx->button0, ctx->layout);
    // x += 50;
    // y += 50;
    // tk::ui::put(ctx->button, ctx->layout, x, y);
    // if (removed)
    // {
    //   tk::ui::put(ctx->button, ctx->layout, 0, 0);
    // }
    // else
    // tk::ui::remove(ctx->button, ctx->layout);
    tk::log::info("button0 is removed!");

    tk::log::info("\n"
                  "x:          {}\n"
                  "y:          {}\n"
                  "width:      {}\n"
                  "height:     {}\n"
                  "window x:   {}\n"
                  "window y:   {}\n"
                  , 
                  ctx->button0->_x, 
                  ctx->button0->_y, 
                  ctx->button0->_width, 
                  ctx->button0->_height,

            ctx->button0->_layout->x,ctx->button0->_layout->x);

  }

  // if (ctx->button1->is_clicked())
  // {
  //   tk::ui::remove(ctx->button1, ctx->layout);
  //   tk::log::info("button1 is removed!");
  // }
}

void tk_event(SDL_Event* event)
{
}

void tk_quit()
{
  delete (Context*)tk::get_user_data();
}
