#include "util/timer.hpp"
#include "error_handling.hpp"

namespace tk {

void Timer::remove_event(uint32_t id) noexcept
{
  err_if(!_events.contains(id), "time event {} is not exist!", id);
  _events.erase(id);
}

auto Timer::add_repeat_event(uint32_t duration, std::function<void()> func, std::function<void(float)> iter_func) noexcept -> uint32_t
{
  err_if(!func, "cannot set empty function in repeat time event");
  auto event = Event{};
  event.id        = Event::generic_id();
  event.type      = Event::Type::repeat;
  event.func      = func;
  event.duratoin  = duration;
  event.iter_func = iter_func;
  _events[event.id] = event;
  _events[event.id].start();
  return event.id;
}

auto Timer::add_single_event(uint32_t duration, std::function<void()> func, std::function<void(float)> iter_func) noexcept -> uint32_t
{
  err_if(!func, "cannot set empty function in repeat time event");
  auto event = Event{};
  event.id        = Event::generic_id();
  event.type      = Event::Type::single;
  event.func      = func;
  event.duratoin  = duration;
  event.iter_func = iter_func;
  _events[event.id] = event;
  _events[event.id].start();
  return event.id;
}

void Timer::process_event(uint32_t id) noexcept
{
  err_if(!_events.contains(id), "time event {} is not exist!", id);
  if (_events.at(id).process())
	_events.erase(id);
}

auto Timer::get_progress(uint32_t id) const noexcept -> float
{
  err_if(!_events.contains(id), "time event {} is not exist!", id);
  return _events.at(id).get_progress();
}

void Timer::set_progress(uint32_t id, float progress) noexcept
{
  err_if(!_events.contains(id), "time event {} is not exist!", id);
  _events.at(id).set_progress(progress);
}

}
