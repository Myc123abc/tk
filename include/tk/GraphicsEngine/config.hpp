#pragma once

namespace tk { namespace graphics_engine {

struct Config
{
  bool use_descriptor_buffer{ true };
  bool use_shader_object{ true };
};

inline auto config() noexcept
{
  static Config cfg;
  return &cfg;
}

}}