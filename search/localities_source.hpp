#pragma once

#include <cstdint>

namespace search
{
struct LocalitiesSource
{
  LocalitiesSource();

  template <typename Fn>
  void ForEachType(Fn && fn) const
  {
    fn(m_city);
    fn(m_town);
  }

  uint32_t m_city = 0;
  uint32_t m_town = 0;
};
}  // namespace search
