#pragma once

#include <string>

namespace software_renderer
{
struct IconInfo
{
  IconInfo() = default;
  explicit IconInfo(std::string const & name) : m_name(name) {}

  std::string m_name;
};
}  // namespace software_renderer
