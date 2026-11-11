#pragma once

#include "map/bookmarks_sync/base_state.hpp"
#include "map/bookmarks_sync/cloud_store.hpp"

#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace bookmarks_sync
{
/// A category file in the bookmarks directory, identified by its file name.
struct LocalFile
{
  std::string m_name;
  std::string m_hash;
};

namespace action
{
/// Replaces the content of m_cloudId if it is unchanged since the last sync, or creates the cloud file when
/// m_cloudId is empty or the file is gone.
struct Upload
{
  std::string m_name;
  std::optional<std::string> m_cloudId;
};

/// Replaces the local file only if its hash is still m_expectedLocalHash; nothing means it must not exist.
/// m_replacedCloudId is the synced cloud file that m_cloud took the name from: the download waits until it is
/// confirmed gone, as a listing can lag behind.
struct Download
{
  std::string m_name;
  CloudFile m_cloud;
  std::optional<std::string> m_expectedLocalHash;
  std::optional<std::string> m_replacedCloudId;
};

/// Keeps the local version as a copy named after the device, then downloads m_cloud under m_name. Without a
/// synchronized state, an untouched list yields to the other version instead.
struct ConflictCopy
{
  std::string m_name;
  CloudFile m_cloud;
  std::string m_localHash;
  bool m_hasBase = false;
};

/// Trashes the cloud file if it is still m_expectedVersion, or forgets it if it is gone.
struct TrashCloud
{
  std::string m_name;
  std::string m_cloudId;
  std::string m_expectedVersion;
};

/// Trashes the local file deleted in the cloud, once m_cloudId is confirmed to be gone.
struct TrashLocal
{
  std::string m_name;
  std::string m_expectedHash;
  std::string m_cloudId;
};

struct RecordBase
{
  std::string m_name;
  BaseEntry m_entry;
};

/// Trashes a cloud file that lost the name to an older one, first downloading it when m_keepAsLocalCopy.
struct ResolveDuplicate
{
  std::string m_name;
  CloudFile m_duplicate;
  bool m_keepAsLocalCopy = false;
};
}  // namespace action

using SyncAction = std::variant<action::ConflictCopy, action::Download, action::Upload, action::TrashCloud,
                                action::TrashLocal, action::RecordBase, action::ResolveDuplicate>;

/// Decides what to do with every file. Cloud files must have m_hash in the algorithm of the local hashes.
/// Copies come first, so that nothing overwrites a local version before it is kept. Duplicates come last: by then
/// the base shows whether the action for their name has kept this device's version.
std::vector<SyncAction> Plan(std::vector<LocalFile> const & local, std::vector<CloudFile> const & cloud,
                             std::map<std::string, BaseEntry> const & base);

/// The file with the earliest creation time, then the smallest id, owns a name.
bool IsOlder(CloudFile const & lhs, CloudFile const & rhs);
}  // namespace bookmarks_sync
