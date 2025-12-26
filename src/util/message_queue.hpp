#pragma once

#include <rigtorp/SPSCQueue.h>

#include <variant>
#include <ranges>

namespace tk {

template <typename Message, uint32_t Capacity>
class MessageQueue
{
public:
  MessageQueue()                               = default;
  ~MessageQueue()                              = default;
  MessageQueue(MessageQueue const&)            = delete;
  MessageQueue(MessageQueue&&)                 = delete;
  MessageQueue& operator=(MessageQueue const&) = delete;
  MessageQueue& operator=(MessageQueue&&)      = delete;

  void send(Message&& msg) noexcept
  {
    _queue.emplace(std::move(msg));
  }

  template <typename Hanlder>
  void process(Hanlder&& handler) noexcept
  {
    for (auto _ : std::views::iota(0u, _queue.size()))
    {
      std::visit(handler, *_queue.front());
      _queue.pop();
    }
  }

private:
  rigtorp::SPSCQueue<Message> _queue{ Capacity };
};

}
