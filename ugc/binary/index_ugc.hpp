#pragma once

#include "ugc/types.hpp"

#include <cstdint>
#include <utility>

namespace ugc
{
namespace binary
{
struct IndexUGC
{
  using Index = uint32_t;

  IndexUGC() = default;

  template <typename U>
  IndexUGC(Index index, U && ugc) : m_index(index), m_ugc(std::forward<U>(ugc))
  {
  }

  Index m_index = 0;
  UGC m_ugc = {};
};
}  // namespace binary
}  // namespace ugc
