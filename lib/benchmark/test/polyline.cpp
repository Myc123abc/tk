#include "polyline_tessellator.hpp"
#include "renderer/resource/shader_type.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <vector>

using namespace tk;

namespace {

constexpr auto Trial_Count        = 9u;
constexpr auto Target_Point_Count = 262'144u;

std::atomic<uint64> benchmark_sink{};

struct Result
{
  double geometry_ns_per_point{};
  double texture_ns_per_point{};
  double time_saving_percent{};
  double geometry_bytes_per_point{};
  double texture_bytes_per_point{};
  double memory_saving_percent{};
};

struct Measurement
{
  double ns_per_point{};
  double bytes_per_point{};
};

auto make_open_points(uint count, bool jagged) -> std::vector<float2>
{
  auto points = std::vector<float2>(count);
  for (auto index = 0u; index < count; ++index)
  {
    auto const x = static_cast<float>(index) * 3.f;
    auto const y = jagged
      ? std::array{ 0.f, 28.f, 4.f, -24.f }[index % 4]
      : std::sin(static_cast<float>(index) * .15f) * 24.f;
    points[index] = { x, y };
  }
  return points;
}

auto make_closed_points(uint count) -> std::vector<float2>
{
  auto points = std::vector<float2>(count);
  for (auto index = 0u; index < count; ++index)
  {
    auto const angle = static_cast<float>(index) * std::numbers::pi_v<float> * 2.f / count;
    auto const radius = index % 2 ? 56.f : 112.f;
    points[index] = { std::cos(angle) * radius, std::sin(angle) * radius };
  }
  return points;
}

auto measure(
  std::span<float2 const>                         points,
  bool                                            is_closed,
  bool                                            stroke_inside,
  tk::ui::PolylineTessellator::FringeMode fringe_mode
) -> Measurement
{
  auto const point_count = static_cast<uint>(points.size());
  auto tessellator = tk::ui::PolylineTessellator{};
  auto const iterations = std::max(16u, Target_Point_Count / point_count);
  auto durations = std::array<double, Trial_Count>{};
  auto mesh = tessellator.build(points, 1.f, 1.f, is_closed, stroke_inside, fringe_mode);

  for (auto trial = 0u; trial < Trial_Count; ++trial)
  {
    auto checksum = uint64{};
    auto const begin = std::chrono::steady_clock::now();
    for (auto iteration = 0u; iteration < iterations; ++iteration)
    {
      mesh = tessellator.build(points, 1.f, 1.f, is_closed, stroke_inside, fringe_mode);
      checksum += mesh.vertices.size() + mesh.indices.size();
    }
    auto const end = std::chrono::steady_clock::now();
    durations[trial] = std::chrono::duration<double, std::nano>(end - begin).count()
      / (static_cast<double>(iterations) * point_count);
    benchmark_sink.fetch_xor(checksum, std::memory_order_relaxed);
  }
  std::ranges::sort(durations);

  auto constexpr Vertex_Size = sizeof(tk::renderer::Vertex);
  auto constexpr Index_Size  = sizeof(uint16);
  auto const bytes = mesh.vertices.size() * Vertex_Size + mesh.indices.size() * Index_Size;
  return
  {
    .ns_per_point    = durations[Trial_Count / 2],
    .bytes_per_point = static_cast<double>(bytes) / point_count,
  };
}

auto benchmark(std::span<float2 const> points, bool is_closed, bool stroke_inside) -> Result
{
  using FringeMode = tk::ui::PolylineTessellator::FringeMode;
  auto const geometry = measure(points, is_closed, stroke_inside, FringeMode::geometry);
  auto const texture  = measure(points, is_closed, stroke_inside, FringeMode::texture_coverage);

  return
  {
    .geometry_ns_per_point    = geometry.ns_per_point,
    .texture_ns_per_point     = texture.ns_per_point,
    .time_saving_percent      = 100. * (geometry.ns_per_point - texture.ns_per_point) / geometry.ns_per_point,
    .geometry_bytes_per_point = geometry.bytes_per_point,
    .texture_bytes_per_point  = texture.bytes_per_point,
    .memory_saving_percent    = 100. * (geometry.bytes_per_point - texture.bytes_per_point) / geometry.bytes_per_point,
  };
}

void print_result(std::string_view shape, uint point_count, Result const& result)
{
  std::cout
    << std::left  << std::setw(14) << shape
    << std::right << std::setw(7) << point_count
    << std::setw(12) << std::fixed << std::setprecision(2) << result.geometry_ns_per_point
    << std::setw(12) << result.texture_ns_per_point
    << std::setw(11) << std::setprecision(1) << result.time_saving_percent
    << std::setw(13) << std::setprecision(2) << result.geometry_bytes_per_point
    << std::setw(13) << result.texture_bytes_per_point
    << std::setw(11) << std::setprecision(1) << result.memory_saving_percent
    << '\n';
}

}

int main()
{
  std::cout
    << std::left  << std::setw(14) << "shape"
    << std::right << std::setw(7) << "points"
    << std::setw(12) << "geo ns/pt"
    << std::setw(12) << "tex ns/pt"
    << std::setw(11) << "time save"
    << std::setw(13) << "geo B/pt"
    << std::setw(13) << "tex B/pt"
    << std::setw(11) << "mem save"
    << '\n';

  for (auto point_count : { 16u, 64u, 256u, 1'024u, 4'096u })
  {
    auto const smooth_points = make_open_points(point_count, false);
    auto const jagged_points = make_open_points(point_count, true);
    auto const closed_points = make_closed_points(point_count);
    print_result("smooth-open", point_count, benchmark(smooth_points, false, false));
    print_result("jagged-open", point_count, benchmark(jagged_points, false, false));
    print_result("star-inside", point_count, benchmark(closed_points, true, true));
  }

  return benchmark_sink.load(std::memory_order_relaxed) == 0;
}
