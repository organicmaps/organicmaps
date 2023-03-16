#pragma once

#include <cstdint>
#include <vector>

namespace search
{
struct LocalitiesSource
{
  LocalitiesSource();

  template <typename Fn>
  void ForEachType(Fn && fn) const
  {
    for (auto const c : m_cities)
      fn(c);
    for (auto const t : m_towns)
      fn(t);
  }

  std::vector<uint32_t> m_cities;
  std::vector<uint32_t> m_towns;
};
}  // namespace search
