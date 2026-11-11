#pragma once

#include "map/bookmarks_sync/cloud_store.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>

namespace bookmarks_sync
{
/// A file after its last successful synchronization: m_hash of the local content, m_version of the cloud file.
struct BaseEntry
{
  std::string m_cloudId;
  std::string m_hash;
  std::string m_version;

  bool operator==(BaseEntry const &) const = default;
};

/// Valid only for the cloud m_generation it was recorded in, and for local hashes in m_algorithm.
/// m_pendingUploads maps names to the hashes of uploads started but not recorded in m_files yet.
struct BaseState
{
  HashAlgorithm m_algorithm = HashAlgorithm::Md5;
  std::optional<std::string> m_generation;
  std::optional<int64_t> m_lastSyncMs;
  std::optional<int64_t> m_lastPurgeMs;
  std::map<std::string, BaseEntry> m_files;
  std::map<std::string, std::string> m_pendingUploads;

  bool operator==(BaseState const &) const = default;
};

/// An empty state in |algorithm| if the file is missing or was recorded for another algorithm.
/// Throws RootException if the file can't be read or parsed.
BaseState LoadBaseState(std::string const & path, HashAlgorithm algorithm);
/// Throws RootException on failure. Writes a temporary file and renames it.
void SaveBaseState(std::string const & path, BaseState const & state);
void DeleteBaseState(std::string const & path);
}  // namespace bookmarks_sync
