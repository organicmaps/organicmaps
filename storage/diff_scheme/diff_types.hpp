#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace diff_scheme
{
enum class Status
{
  Undefined,
  NotAvailable,
  Available
};

struct FileInfo final
{
  FileInfo(uint64_t size, uint64_t version) : m_size(size), m_version(version) {}
  uint64_t m_size;
  uint64_t m_version;
};

using NameFileInfoMap = std::unordered_map<std::string, FileInfo>;
}  // namespace diff_scheme
