#pragma once

#include "storage/diff_scheme/diff_types.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

namespace diff_scheme
{
class Checker final
{
public:
  using NameVersionMap = std::unordered_map<std::string, uint64_t>;

  struct LocalMapsInfo final
  {
    uint64_t m_currentDataVersion = 0;
    NameVersionMap m_localMaps;
  };

  using Callback = std::function<void(NameFileInfoMap const & diffs)>;

  static void Check(LocalMapsInfo const & info, Callback const & fn);
};
}  // namespace diff_scheme
