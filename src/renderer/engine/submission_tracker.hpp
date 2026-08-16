#pragma once

#include "../../util/singleton.hpp"
#include "engine.hpp"

namespace tk::renderer {

Singleton(SubmissionTracker, g_sub_tracker,

public:
  void reset() noexcept
  {
    std::erase_if(_records, [](auto const& record)
    {
      return record.engine->fence_completed_value() >= record.fence_value;
    });
  }

  auto submit(Engine& engine) noexcept
  {
    auto cmds = std::vector{ engine.cmd() };
    auto usages = std::vector<GPUResourceUsage>{};
    for (auto cmd : cmds)
      usages.append_range(cmd->usages());

    // Wait for engines used by resources in the submitted commands
    wait_engines(engine, usages);

    auto fence_value = engine.submit(cmds);

    // Record engine usage
    if (!usages.empty())
      _records.emplace_back(&engine, std::move(usages), fence_value);

    for (auto cmd : cmds)
      cmd->clear_usages();

    return fence_value;
  }

private:
  static auto is_resource_used(std::span<GPUResourceUsage const> previous,
                               std::span<GPUResourceUsage const> current) noexcept
  {
    for (auto const& usage : previous)
      for (auto const& current_usage : current)
        if (needs_sync(usage, current_usage))
          return true;
    return false;
  }

  void wait_engines(Engine const& engine, std::span<GPUResourceUsage const> usages) noexcept
  {
    if (_records.empty() || usages.empty()) return;

    auto waits = std::unordered_map<Engine*, uint64>{};

    // Find previous records that contain resources used by the current commands
    for (auto const& record : _records)
    {
      // Only different engines need to wait
      if (record.engine == &engine) continue;
      
      // If a resource is used by the current commands, update the maximum fence value for this engine
      if (is_resource_used(record.usages, usages))
      {
        auto& fence_value = waits[record.engine];
        fence_value = std::max(fence_value, record.fence_value);
      }
    }
    
    // Wait for the required engines
    for (auto [other_engine, fence_value] : waits)
      engine.wait(*other_engine, fence_value);
  }

private:
  struct SubmitRecord
  {
    Engine*                       engine{};
    std::vector<GPUResourceUsage> usages;
    uint64                        fence_value{};
  };
  std::vector<SubmitRecord> _records;
)

}
