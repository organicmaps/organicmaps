#pragma once

#include "storage/storage_defines.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace storage
{
namespace diffs
{
// Status of the diffs data source as a whole.
enum class Status
{
  NotAvailable,
  Available
};

struct DiffInfo final
{
  DiffInfo(uint64_t size, uint64_t version) : m_size(size), m_version(version) {}

  uint64_t m_size;
  uint64_t m_version;
  bool m_isApplied = false;
};

using NameDiffInfoMap = std::unordered_map<storage::CountryId, DiffInfo>;
}  // namespace diffs
}  // namespace storage
