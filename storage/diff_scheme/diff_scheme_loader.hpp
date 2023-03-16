#pragma once

#include "storage/diff_scheme/diff_types.hpp"
#include "storage/storage_defines.hpp"

#include <cstdint>
#include <functional>
#include <unordered_map>

namespace storage
{
namespace diffs
{
struct LocalMapsInfo final
{
  using NameVersionMap = std::unordered_map<storage::CountryId, uint64_t>;

  uint64_t m_currentDataVersion = 0;
  NameVersionMap m_localMaps;
};

using DiffsReceivedCallback = std::function<void(diffs::NameDiffInfoMap && diffs)>;

class Loader final
{
public:
  static void Load(LocalMapsInfo && info, DiffsReceivedCallback && callback);
};
}  // namespace diffs
}  // namespace storage
