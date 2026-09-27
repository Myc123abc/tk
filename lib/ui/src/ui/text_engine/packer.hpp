#pragma once

#include "tk/base.hpp"

#include <vector>
#include <cassert>

namespace tk::ui {

class SkylinePacker
{
public:
  SkylinePacker()                                = default;
  ~SkylinePacker()                               = default;
  SkylinePacker(SkylinePacker const&)            = delete;
  SkylinePacker(SkylinePacker&&)                 = delete;
  SkylinePacker& operator=(SkylinePacker const&) = delete;
  SkylinePacker& operator=(SkylinePacker&&)      = delete;

  void reset(float2 ext) noexcept
  {
    _nodes.clear();
    _nodes.emplace_back(0, 0, ext.x);
    _img_ext = ext;
  }

  auto add(float width, float height) noexcept -> std::optional<float2>
  {
    assert(!_nodes.empty());

    auto best_height = std::numeric_limits<float>::max();
    auto best_width  = std::numeric_limits<float>::max();
    auto best_x      = 0.f;
    auto best_y      = 0.f;
    auto target      = -1;
    for (auto i = 0; i < _nodes.size(); ++i)
    {
      // Checks whether current node have enough space
      if (auto y = node_available(i, width, height); y)
      {
        // Use Bottom-Left Heuristic
        auto curr_height = y.value() + height;
        if ((curr_height < best_height) ||
            (curr_height == best_height && _nodes[i].width < best_width))
        {
          best_height = curr_height;
          best_width  = _nodes[i].width;
          best_x      = _nodes[i].x;
          best_y      = y.value();
          target      = i;
        }
      }
    }

    if (target == -1) return {};

    add_node(target, best_x, best_y, width, height);
    merge_nodes();

    return float2{ best_x, best_y };
  }

private:
  auto node_available(uint idx, float width, float height) -> std::optional<uint>
  {
    // If put rect exceed the atlas width, unavailable.
    if (_nodes[idx].x + width > _img_ext.x) return {};

    auto y = 0.f;
    // Checks continues nodes because the rect can put on multiple nodes.
    for (; width > 0 && idx < _nodes.size(); ++idx)
    {
      // Get current y of position of put rect
      y = std::max(y, _nodes[idx].y);
      // If put rect exceed the atlas height, unavailable.
      if (y + height > _img_ext.y) return {};
      // Calculate remain width for next node usage
      width -= _nodes[idx].width;
    }

    // If all remain nodes be used but still unenough, unavailable.
    if (width > 0) return {};

    return y;
  }

  struct SkylineNode
  {
    float x{};
    float y{};
    float width{};

    auto end_x() const noexcept { return x + width; }
  };

  void add_node(uint idx, float x, float y, float width, float height) noexcept
  {
    _nodes.insert(_nodes.begin() + idx, { x, y + height, width });

    // Shrink or remove nodes underneath the inserted rect
    for (auto i = idx + 1; i < _nodes.size(); ++i)
    {
      auto& node = _nodes[i];
      auto prev_end_x = _nodes[i - 1].end_x();

      if (node.x < prev_end_x)
      {
        auto overlap = prev_end_x - node.x;
        node.x     += overlap;
        node.width -= overlap;

        if (node.width <= 0)
        {
          _nodes.erase(_nodes.begin() + i);
          --i;
        }
      }
    }
  }

  void merge_nodes() noexcept
  {
    for (auto i = 0; i < _nodes.size() - 1; ++i)
    {
      if (_nodes[i].y == _nodes[i + 1].y)
      {
        _nodes[i].width += _nodes[i + 1].width;
        _nodes.erase(_nodes.begin() + i + 1);
        --i;
      }
    }
  }

private:
  std::vector<SkylineNode> _nodes;
  float2                   _img_ext;
};

}
