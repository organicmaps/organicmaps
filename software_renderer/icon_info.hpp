#pragma once

#include "std/string.hpp"

namespace software_renderer
{

struct IconInfo
{
  string m_name;

  IconInfo() = default;
  explicit IconInfo(string const & name) : m_name(name) {}
};

}
