#pragma once

#include "map/bookmarks_sync/cloud_store.hpp"
#include "map/bookmarks_sync/local_store.hpp"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>

class BookmarkManager;

namespace bookmarks_sync
{
struct PassResult
{
  enum class Status
  {
    Synced,
    // Some local changes wait until no screen keeps their categories.
    Deferred,
    // Something changed while the pass ran, so another pass is needed soon.
    Retry,
    Failed,
  };

  Status m_status = Status::Synced;
  SyncError m_error = SyncError::Other;
  std::string m_message;
};

std::string DebugPrint(PassResult::Status status);
std::string DebugPrint(PassResult const & result);

/// Synchronizes the category files of the bookmarks directory with a cloud store. Runs one pass at a time and
/// persists the base state after every applied action, so an interrupted pass continues safely next time.
/// Calls block and wait for the GUI and File threads: make them on another thread. Cancel(), IsRunning() and
/// GetLastSyncMs() don't block.
class SyncEngine
{
public:
  /// Local files are hashed in |cloudAlgorithm| when the store reports hashes, in Blake3 otherwise.
  SyncEngine(BookmarkManager & manager, std::string bookmarksDir, std::string statePath, std::string stagingDir,
             std::string deviceName, std::optional<HashAlgorithm> cloudAlgorithm);

  PassResult RunPass(CloudStore & cloud, CanApply const & canApply);

  /// Deletes the generation of the base first: a failure before it leaves the base matching the cloud data, and
  /// the next pass after it starts a new generation, so a partial deletion neither copies nor deletes a list.
  /// Throws CloudException.
  void DeleteCloudData(CloudStore & cloud);

  /// Keeps the base only for files that still exist here, so that a list deleted while sync was off is downloaded
  /// again instead of deleted in the cloud.
  void ForgetMissingLocalFiles();

  /// The running pass stops before its next cloud call and fails with SyncError::Cancelled.
  void Cancel() { m_cancelled = true; }
  bool IsRunning() const { return m_running; }
  std::optional<int64_t> GetLastSyncMs() const;

private:
  friend class Pass;

  BookmarkManager & m_manager;
  std::string const m_bookmarksDir;
  std::string const m_statePath;
  std::string const m_stagingDir;
  std::string const m_deviceName;
  std::optional<HashAlgorithm> const m_cloudAlgorithm;
  HashAlgorithm const m_algorithm;

  std::mutex m_mutex;
  std::atomic<bool> m_cancelled = false;
  std::atomic<bool> m_running = false;
  // 0 if never synchronized.
  std::atomic<int64_t> m_lastSyncMs = 0;
};
}  // namespace bookmarks_sync
