#pragma once

#include "map/gps_track_collection.hpp"
#include "map/gps_track_filter.hpp"
#include "map/gps_track_storage.hpp"

#include "base/thread.hpp"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

class GpsTrack final
{
public:
  static size_t const kInvalidId; // = numeric_limits<size_t>::max();

  /// @param filePath - path to the file on disk to persist track
  /// @param maxItemCount - number of points to store on disk
  /// @param duration - initial value of track duration
  /// @param filter - filter object used for filtering points, GpsTrackNullFilter is created by default
  GpsTrack(std::string const & filePath, size_t maxItemCount, std::chrono::hours duration,
           std::unique_ptr<IGpsTrackFilter> && filter = std::unique_ptr<IGpsTrackFilter>());
  ~GpsTrack();

  /// Adds point or collection of points to gps tracking
  /// @note Callback is called with 'toAdd' and 'toRemove' points, if some points were added or removed.
  /// @note Only points with good timestamp will be added, other will be skipped.
  void AddPoint(location::GpsInfo const & point);
  void AddPoints(std::vector<location::GpsInfo> const & points);

  /// Clears any previous tracking info
  /// @note Callback is called with 'toRemove' points, if some points were removed.
  void Clear();

  /// Sets tracking duration in hours.
  /// @note Callback is called with 'toRemove' points, if some points were removed.
  /// By default, duration is 24h.
  void SetDuration(std::chrono::hours duration);

  /// Returns track duraion in hours
  std::chrono::hours GetDuration() const;

  /// Notification callback about a change of the gps track.
  /// @param toAdd - collection of points and ids to add.
  /// @param toRemove - range of point indices to remove, or pair(kInvalidId,kInvalidId) if nothing to remove
  /// @note Calling of a GpsTrack.SetCallback function from the callback causes deadlock.
  using TGpsTrackDiffCallback =
      std::function<void(std::vector<std::pair<size_t, location::GpsTrackInfo>> && toAdd,
                         std::pair<size_t, size_t> const & toRemove)>;

  /// Sets callback on change of gps track.
  /// @param callback - callback callable object
  /// @note Only one callback is supported at time.
  /// @note When sink is attached, it receives all points in 'toAdd' at first time,
  /// next time callbacks it receives only modifications. It simplifies getter/callback model.
  void SetCallback(TGpsTrackDiffCallback callback);

private:
  DISALLOW_COPY_AND_MOVE(GpsTrack);

  void ScheduleTask();
  void ProcessPoints(); // called on the worker thread
  bool HasCallback();
  void InitStorageIfNeed();
  void InitCollection(std::chrono::hours duration);
  void UpdateStorage(bool needClear, std::vector<location::GpsInfo> const & points);
  void UpdateCollection(std::chrono::hours duration, bool needClear,
                        std::vector<location::GpsTrackInfo> const & points,
                        std::pair<size_t, size_t> & addedIds,
                        std::pair<size_t, size_t> & evictedIds);
  void NotifyCallback(std::pair<size_t, size_t> const & addedIds,
                      std::pair<size_t, size_t> const & evictedIds);

  size_t const m_maxItemCount;
  std::string const m_filePath;

  mutable std::mutex m_dataGuard;           // protects data for stealing
  std::vector<location::GpsInfo> m_points;  // accumulated points for adding
  std::chrono::hours m_duration;
  bool m_needClear; // need clear file

  std::mutex m_callbackGuard;
  // Callback is protected by m_callbackGuard. It ensures that SetCallback and call callback
  // will not be interleaved and after SetCallback(null) callbakc is never called. The negative side
  // is that GpsTrack.SetCallback must be never called from the callback.
  TGpsTrackDiffCallback m_callback;
  bool m_needSendSnapshop; // need send initial snapshot

  std::unique_ptr<GpsTrackStorage> m_storage;        // used in the worker thread
  std::unique_ptr<GpsTrackCollection> m_collection;  // used in the worker thread
  std::unique_ptr<IGpsTrackFilter> m_filter;         // used in the worker thread

  std::mutex m_threadGuard;
  threads::SimpleThread m_thread;
  bool m_threadExit; // need exit thread
  bool m_threadWakeup; // need wakeup thread
  std::condition_variable m_cv;
};
