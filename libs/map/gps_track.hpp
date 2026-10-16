#pragma once

#include "map/gps_track_collection.hpp"
#include "map/gps_track_filter.hpp"
#include "map/gps_track_storage.hpp"

#include "base/thread.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

/// Lifecycle calls (Start and Finalize) must be serialized by the caller.
class GpsTrack final
{
public:
  static size_t constexpr kInvalidId = GpsTrackCollection::kInvalidId;

  /// @param filePath - path to the crash backup, written in batches while recording
  /// @param filter - filter object used for filtering points, GpsTrackNullFilter is created by default
  /// @param backupInterval - interval for backing up pending fixes
  GpsTrack(std::string const & filePath, std::unique_ptr<IGpsTrackFilter> && filter = {},
           std::chrono::milliseconds backupInterval = std::chrono::minutes(1));
  ~GpsTrack();

  /// Starts recording, restoring a previous crash backup if present.
  void Start();

  /// Adds point or collection of points to gps tracking
  /// @note Callback is called with 'toAdd' and 'toRemove' points, if some points were added or removed.
  /// @note Only points with good timestamp will be added, other will be skipped.
  void AddPoint(location::GpsInfo const & point);
  void AddPoints(std::vector<location::GpsInfo> const & points);

  /// Returns track statistics
  TrackStatistics GetTrackStatistics() const;
  /// Returns an owned profile. Graph processing runs outside the recording lock.
  ElevationInfo GetElevationInfo() const;

  using IsCancelled = std::function<bool()>;
  struct ElevationSnapshot
  {
    uint64_t m_epoch;
    size_t m_revision;
    TrackStatistics m_statistics;
    ElevationInfo m_elevation;
  };
  /// Safe on a background thread. An unchanged revision reuses the same immutable snapshot.
  std::shared_ptr<ElevationSnapshot const> GetElevationSnapshot(IsCancelled const & isCancelled = {}) const;

  /// Discards previous tracking info and removes the crash backup.
  /// @note Callback is called with 'toRemove' points, if some points were removed.
  void Clear();

  bool IsEmpty() const;

  /// Notification callback about a change of the gps track.
  /// @param toAdd - collection of points and ids to add.
  /// @param toRemove - range of point indices to remove, or pair(kInvalidId,kInvalidId) if nothing to remove
  /// @note Calling of a GpsTrack.SetCallback function from the callback causes deadlock.
  using TGpsTrackDiffCallback = std::function<void(std::vector<std::pair<size_t, location::GpsInfo>> && toAdd,
                                                   std::pair<size_t, size_t> const & toRemove)>;

  /// Sets callback on change of gps track.
  /// @param callback - callback callable object
  /// @note Only one callback is supported at time.
  /// @note When sink is attached, it receives all points in 'toAdd' at first time,
  /// next time callbacks it receives only modifications. It simplifies getter/callback model.
  void SetCallback(TGpsTrackDiffCallback callback);

  /// @note Calling SetStatisticsCallback from the callback causes deadlock.
  using TGpsTrackStatisticsCallback = std::function<void(TrackStatistics const & trackStatistics)>;

  /// Called on the worker thread with the current statistics, then on each change.
  void SetStatisticsCallback(TGpsTrackStatisticsCallback callback);

  /// Finishes recording and flushes its backup, keeping it until Clear.
  size_t Finalize();

  /// @pre Finalize should be called before.
  template <class FnT>
  void ForEachPoint(FnT && fn)
  {
    m_collection.ForEach(fn);
  }

private:
  DISALLOW_COPY_AND_MOVE(GpsTrack);

  // Include Android device suspend time when deciding whether a backup is due.
  using Clock = std::chrono::system_clock;

  // Called with m_threadGuard held.
  void ScheduleTask();
  void ProcessPoints();
  void SaveBackup();
  void ClearBackup();
  void NotifyCallback(std::pair<size_t, size_t> const & addedIds, std::pair<size_t, size_t> const & evictedIds);
  void NotifyStatisticsCallback(bool changed);

  std::chrono::milliseconds const m_backupInterval;

  std::mutex m_callbackGuard;
  // Callback is protected by m_callbackGuard. It ensures that SetCallback and call callback
  // will not be interleaved and after SetCallback(null) callback is never called. The negative side
  // is that GpsTrack.SetCallback must be never called from the callback.
  TGpsTrackDiffCallback m_callback;
  TGpsTrackStatisticsCallback m_statisticsCallback;
  bool m_needSendSnapshot = false;
  bool m_needSendStatistics = false;

  // Recording state is protected by m_threadGuard; callbacks have a separate guard.
  mutable std::mutex m_threadGuard;
  GpsTrackStorage m_storage;
  GpsTrackCollection m_collection;
  std::unique_ptr<IGpsTrackFilter> m_filter;
  std::vector<location::GpsInfo> m_points;
  std::vector<location::GpsInfo> m_backupPoints;
  Clock::time_point m_nextBackup;
  bool m_recording = false;
  bool m_needRestore = false;
  bool m_needClear = false;
  threads::SimpleThread m_thread;
  bool m_threadExit = false;
  bool m_threadWakeup = false;
  std::condition_variable m_cv;

  std::atomic<uint64_t> m_epoch = 0;  // cancels graph replay synchronously on Clear() and Start()
  struct ElevationCache;
  mutable std::mutex m_elevationGuard;
  mutable std::unique_ptr<ElevationCache> m_elevationCache;
};
