#pragma once

#include "storage/storage_defines.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace storage
{
namespace diffs
{
enum class Status
{
  Undefined,
  NotAvailable,
  Available
};

struct DiffInfo final
{
  DiffInfo(uint64_t size, uint64_t version) : m_size(size), m_version(version) {}
  uint64_t m_size;
  uint64_t m_version;
  bool m_applied = false;
  bool m_downloaded = false;
};

using NameDiffInfoMap = std::unordered_map<storage::CountryId, DiffInfo>;

struct LocalMapsInfo final
{
  using NameVersionMap = std::unordered_map<storage::CountryId, uint64_t>;

  uint64_t m_currentDataVersion = 0;
  NameVersionMap m_localMaps;
};
}  // namespace diffs
}  // namespace storage
