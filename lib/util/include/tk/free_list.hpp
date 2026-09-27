namespace tk {

class FreeList
{
public:
  void push(void* p) noexcept
  {
    auto node = static_cast<Node*>(p);
    node->next = _head;
    _head = node;
  }

  auto pop() noexcept -> void*
  {
    if (!_head) return nullptr;
    auto p = _head;
    _head = _head->next;
    return p;
  }

  void clear() noexcept
  {
    _head = {};
  }

private:
  struct Node
  {
    Node* next{};
  };
  Node* _head{};
};

}
