#include "internal.hpp"
#include "tk/ErrorHandling.hpp"

#include <algorithm>
#include <ranges>

namespace tk { namespace ui {

// HACK: background should not have mouse down handling
//       ui UIWidget to handle to bad
// 
// HACK: should not have multi uis in same place have same depth value

auto get_mouse_over_widget() -> ClickableWidget*
{
  auto& widgets = get_ctx().widgets;
  auto hovered_widgets = widgets |
    std::views::filter([](auto const& widget)
    {
      return (uint32_t)widget->get_type() & (uint32_t)UIType::ClickableWidget &&
             dynamic_cast<ClickableWidget*>(widget.get())->is_mouse_over(); 
    });

  auto it = std::ranges::max_element(hovered_widgets, 
    [](auto const& l, auto const& r)
    {
      return l->get_depth() < r->get_depth();
    });

  // INFO: test whether multi-widgets have same depth value
  //       if yes, throw exception, now we not to handle this case
  float depth;
  if (it != hovered_widgets.end())
    depth = it->get()->get_depth();
  else
    return {};

  auto max_depth_widgets = hovered_widgets |
    std::views::filter([depth](auto const& widget)
    {
      return widget->get_depth() == depth;
    });
  throw_if(std::ranges::distance(max_depth_widgets) > 1, "multiple ui widgets have same depth value");

  return dynamic_cast<ClickableWidget*>(it->get());
}

void set_widget_is_clicked()
{
  auto ctx = get_ctx();
  if (ctx.mouse_down_widget == nullptr)
    return;
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
    auto widget = get_mouse_over_widget();
    // when mouse is not on the mouse down widget
    if (widget != ctx.mouse_down_widget)
    {
      // clear mouse down widget
      ctx.mouse_down_widget = nullptr;
    }
  }

  switch (event->type)
  {
  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    ctx.mouse_down_widget = get_mouse_over_widget();
    break;
  case SDL_EVENT_MOUSE_BUTTON_UP:
    set_widget_is_clicked();
    break;
  }
}


}}
