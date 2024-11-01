#pragma once

#include <cstdint>
#include <sstream>
#include <string>

namespace om::network
{
struct Progress
{
  static int64_t constexpr kUnknownTotalSize = -1;

  static Progress constexpr Unknown() { return {0, kUnknownTotalSize}; }

  bool IsUnknown() const { return m_bytesTotal == kUnknownTotalSize; }

  int64_t m_bytesDownloaded = 0;
  /// Total can be kUnknownTotalSize if size is unknown.
  int64_t m_bytesTotal = 0;
};

inline std::string DebugPrint(Progress const & progress)
{
  std::ostringstream out;
  out << "(downloaded " << progress.m_bytesDownloaded << " bytes out of " << progress.m_bytesTotal << " bytes)";
  return out.str();
}
}  // namespace om::network
