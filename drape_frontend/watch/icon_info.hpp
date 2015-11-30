#pragma once

#include "std/string.hpp"

namespace df
{
namespace watch
{

struct IconInfo
{
  string m_name;

  IconInfo() = default;
  explicit IconInfo(string const & name) : m_name(name) {}
};

}
}
