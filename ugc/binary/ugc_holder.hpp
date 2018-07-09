#pragma once

#include "ugc/binary/index_ugc.hpp"

#include <cstdint>
#include <utility>
#include <vector>

namespace ugc
{
namespace binary
{
// Wrapper used to collect pairs (feature id, ugcs).
struct UGCHolder
{
  template <typename U>
  void Add(uint32_t index, U && ugc)
  {
    m_ugcs.emplace_back(index, std::forward<U>(ugc));
  }

  std::vector<IndexUGC> m_ugcs;
};
}  // namespace binary
}  // namespace ugc
