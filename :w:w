#include "internal.hpp"

#include "tk/log.hpp"

#include <ranges>

namespace tk { namespace ui {

// HACK: background should not have mouse down handling
//       ui UIWidget to handle to bad
// 
// HACK: should not have multi uis in same place have same depth value

auto get_mouse_over_widget() -> UIWidget*
{
  auto res = get_ctx().widgets |
    std::views::filter([](auto const& widget)
    {
      return widget->is_mouse_over(); 
    });
  auto it = std::ranges::max_element(res, 
    [](auto const& l, auto const& r)
    {
      return l->get_depth() < r->get_depth();
    });
  return it != res.end() ? it->get() : nullptr;
}

void set_widget_is_clicked()
{
  auto ctx = get_ctx();

  if (ctx.mouse_down_widget == nullptr)
  {
    log::info("invalid widget for mouse down");
    return;
  }

  log::info("be clicked");
  ctx.mouse_down_widget->set_is_clicked();
  ctx.mouse_down_widget = nullptr;
}

void event_process(SDL_Event* event)
{
  // when have mouse down widget
  // check whether mouse leave the widget extent
  auto ctx = get_ctx();
  if (ctx.mouse_down_widget)
  {
    log::info("have mouse down");
    auto widget = get_mouse_over_widget();
    // when mouse is not on the mouse down widget
    if (widget != ctx.mouse_down_widget)
    {
      log::info("mouse leave");
      // clear mouse down widget
      ctx.mouse_down_widget = nullptr;
    }
  }

  switch (event->type)
  {
  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    log::info("mouse down");
    ctx.mouse_down_widget = get_mouse_over_widget();
    log::info("mouse down widget {}", (void*)ctx.mouse_down_widget);
    break;
  case SDL_EVENT_MOUSE_BUTTON_UP:
    log::info("mouse up");
    set_widget_is_clicked();
    break;
  }
}


}}
