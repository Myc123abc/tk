#pragma once

#include "tk/base.hpp"

#include <span>

namespace tk::ui {

struct TriangulatorNode
{
  enum Type
  {
    convex,
    ear,
    reflex,
  };

  Type              type{};
  uint              idx{};
  float2            pos;
  TriangulatorNode* next{};
  TriangulatorNode* prev{};

  void unlink() noexcept { next->prev = prev; prev->next = next; }
};

struct TriangulatorNodeSpan
{
  TriangulatorNode** data{};
  uint               size{};

  void push(TriangulatorNode* node) noexcept { data[size++] = node; }
  void erase(uint idx) noexcept
  {
    for (int i = size - 1; i >= 0; --i)
    {
      if (data[i]->idx == idx)
      {
        data[i] = data[size - 1];
        --size;
        return;
      }
    }
  }
};

struct Triangulator
{
  static auto estimate_triangle_cnt(uint pt_cnt) noexcept { return (pt_cnt < 3) ? 0 : pt_cnt - 2; }
  static auto estimate_buf_size(uint pt_cnt) noexcept { return sizeof(TriangulatorNode) * pt_cnt + sizeof(TriangulatorNode*) * pt_cnt * 2; }

  void init(std::span<float2> points, void* buf) noexcept
  {
    assert(buf != nullptr && points.size() > 2);
    _triangles_left = estimate_triangle_cnt(points.size());
    _nodes          = reinterpret_cast<TriangulatorNode*>(buf);
    _ears.data      = reinterpret_cast<TriangulatorNode**>(_nodes + points.size());
    _reflexs.data   = reinterpret_cast<TriangulatorNode**>(_nodes + points.size()) + points.size();
    build_nodes(points);
    build_reflexes();
    build_ears();
  }

  auto triangle_left() const noexcept { return _triangles_left; }

  auto get_next_triangle(uint triangle[3]) noexcept
  {
    if (_ears.size == 0)
    {
      flip_nodes();

      auto node = _nodes;
      for (int i = _triangles_left; i >= 0; --i, node = node->next)
        node->type = TriangulatorNode::Type::convex;
      _reflexs.size = 0;
      build_reflexes();
      build_ears();

      if (_ears.size == 0)
      {
        assert(_triangles_left > 0);
        _ears.data[0] = _nodes;
        _ears.size    = 1;
      }
    }
    
    auto ear = _ears.data[--_ears.size];
    triangle[0] = ear->prev->idx;
    triangle[1] = ear->idx;
    triangle[2] = ear->next->idx;

    ear->unlink();
    if (ear == _nodes)
      _nodes = ear->next;

    reclassify(ear->prev);
    reclassify(ear->next);
    --_triangles_left;
  }

private:
  void build_nodes(std::span<float2> points) noexcept
  {
    for (auto i = 0u; i < points.size(); ++i)
      _nodes[i] =
      {
        .type = TriangulatorNode::Type::convex,
        .idx  = i,
        .pos  = points[i],
        .next = _nodes + i + 1,
        .prev = _nodes + i - 1,
      };
    _nodes[0].prev = _nodes + points.size() - 1;
    _nodes[points.size() - 1].next = _nodes;
  }

  static auto is_clock_wise(float2 a, float2 b, float2 c) noexcept
  {
    return ((b.x - a.x) * (c.y - b.y)) - ((c.x - b.x) * (b.y - a.y)) > 0.f;
  }

  void build_reflexes() noexcept
  {
    auto n1 = _nodes;
    for (int i = _triangles_left; i >= 0; --i, n1 = n1->next)
    {
      if (is_clock_wise(n1->prev->pos, n1->pos, n1->next->pos))
        continue;
      n1->type = TriangulatorNode::Type::reflex;
      _reflexs.push(n1);
    }
  }

  static auto triangle_contains(float2 a, float2 b, float2 c, float2 p) noexcept
  {
    auto b1 = ((p.x - b.x) * (a.y - b.y) - (p.y - b.y) * (a.x - b.x)) < 0.0f;
    auto b2 = ((p.x - c.x) * (b.y - c.y) - (p.y - c.y) * (b.x - c.x)) < 0.0f;
    auto b3 = ((p.x - a.x) * (c.y - a.y) - (p.y - a.y) * (c.x - a.x)) < 0.0f;
    return (b1 == b2) && (b2 == b3);
  }

  auto is_ear(int i0, int i1, int i2, float2 v0, float2 v1, float2 v2) const noexcept
  {
    auto p_end = _reflexs.data + _reflexs.size;
    for (auto p = _reflexs.data; p < p_end; ++p)
    {
      auto reflex = *p;
      if (reflex->idx != i0 && reflex->idx != i1 && reflex->idx != i2)
        if (triangle_contains(v0, v1, v2, reflex->pos))
          return false;
    }
    return true;
  }

  void build_ears() noexcept
  {
    auto n1 = _nodes;
    for (int i = _triangles_left; i >= 0; --i, n1 = n1->next)
    {
      if (n1->type != TriangulatorNode::Type::convex)
        continue;
      if (!is_ear(n1->prev->idx, n1->idx, n1->next->idx, n1->prev->pos, n1->pos, n1->next->pos))
        continue;
      n1->type = TriangulatorNode::Type::ear;
      _ears.push(n1);
    }
  }

  void flip_nodes() noexcept
  {
    auto prev = _nodes;
    auto tmp  = _nodes;
    auto cur  = _nodes->next;
    prev->next = prev;
    prev->prev = prev;
    while (cur != _nodes)
    {
      tmp = cur->next;

      cur->next    = prev;
      prev->prev   = cur;
      _nodes->next = cur;
      cur->prev    = _nodes;

      prev = cur;
      cur  = tmp;
    }
    _nodes = prev;
  }

  void reclassify(TriangulatorNode* n1) noexcept
  {
    auto type = TriangulatorNode::Type{};
    auto const n0 = n1->prev;
    auto const n2 = n1->next;
    if (!is_clock_wise(n0->pos, n1->pos, n2->pos))
      type = TriangulatorNode::Type::reflex;
    else if (is_ear(n0->idx, n1->idx, n2->idx, n0->pos, n1->pos, n2->pos))
      type = TriangulatorNode::Type::ear;
    else
      type = TriangulatorNode::Type::convex;

    if (type == n1->type)
      return;
    if (n1->type == TriangulatorNode::Type::reflex)
      _reflexs.erase(n1->idx);
    else if (n1->type == TriangulatorNode::Type::ear)
      _ears.erase(n1->idx);
    if (type == TriangulatorNode::Type::reflex)
      _reflexs.push(n1);
    else if (type == TriangulatorNode::Type::ear)
      _ears.push(n1);
    n1->type = type;
  }

private:
  uint                 _triangles_left{};
  TriangulatorNode*    _nodes{};
  TriangulatorNodeSpan _ears{};
  TriangulatorNodeSpan _reflexs{};
};

}
