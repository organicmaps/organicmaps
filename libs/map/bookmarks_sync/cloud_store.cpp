#include "map/bookmarks_sync/cloud_store.hpp"

#include "base/assert.hpp"
#include "base/internal/message.hpp"

namespace bookmarks_sync
{
std::string DebugPrint(HashAlgorithm algorithm)
{
  switch (algorithm)
  {
  case HashAlgorithm::Md5: return "Md5";
  case HashAlgorithm::Blake3: return "Blake3";
  }
  UNREACHABLE();
}

std::string DebugPrint(CloudFile const & file)
{
  return "CloudFile [" + file.m_name + ", " + file.m_id + ", " + file.m_version + ", " + ::DebugPrint(file.m_hash) +
         ", " + std::to_string(file.m_createdTimeMs) + "]";
}

std::string DebugPrint(SyncError error)
{
  switch (error)
  {
  case SyncError::AuthRequired: return "AuthRequired";
  case SyncError::QuotaExceeded: return "QuotaExceeded";
  case SyncError::Transient: return "Transient";
  case SyncError::Cancelled: return "Cancelled";
  case SyncError::Other: return "Other";
  }
  UNREACHABLE();
}
}  // namespace bookmarks_sync
