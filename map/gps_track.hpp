#pragma once

#include "map/gps_track_collection.hpp"
#include "map/gps_track_filter.hpp"
#include "map/gps_track_storage.hpp"

#include "base/thread.hpp"

#include "std/condition_variable.hpp"
#include "std/mutex.hpp"
#include "std/unique_ptr.hpp"

class GpsTrack final
{
public:
  static size_t const kInvalidId; // = numeric_limits<size_t>::max();

  /// @param filePath - path to the file on disk to persist track
  /// @param maxItemCount - number of points to store on disk
  /// @param duration - initial value of track duration
  /// @param filter - filter object used for filtering points, GpsTrackNullFilter is created by default
  GpsTrack(string const & filePath, size_t maxItemCount, hours duration,
           unique_ptr<IGpsTrackFilter> && filter = unique_ptr<IGpsTrackFilter>());
  ~GpsTrack();

  /// Adds point or collection of points to gps tracking
  /// @note Callback is called with 'toAdd' and 'toRemove' points, if some points were added or removed.
  /// @note Only points with good timestamp will be added, other will be skipped.
  void AddPoint(location::GpsInfo const & point);
  void AddPoints(vector<location::GpsInfo> const & points);

  /// Clears any previous tracking info
  /// @note Callback is called with 'toRemove' points, if some points were removed.
  void Clear();

  /// Sets tracking duration in hours.
  /// @note Callback is called with 'toRemove' points, if some points were removed.
  /// By default, duration is 24h.
  void SetDuration(hours duration);

  /// Returns track duraion in hours
  hours GetDuration() const;

  /// Notification callback about a change of the gps track.
  /// @param toAdd - collection of points and ids to add.
  /// @param toRemove - range of point indices to remove, or pair(kInvalidId,kInvalidId) if nothing to remove
  /// @note Calling of a GpsTrack.SetCallback function from the callback causes deadlock.
  using TGpsTrackDiffCallback = std::function<void(vector<pair<size_t, location::GpsTrackInfo>> && toAdd,
                                                   pair<size_t, size_t> const & toRemove)>;

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
  void InitCollection(hours duration);
  void UpdateStorage(bool needClear, vector<location::GpsInfo> const & points);
  void UpdateCollection(hours duration, bool needClear, vector<location::GpsTrackInfo> const & points,
                        pair<size_t, size_t> & addedIds, pair<size_t, size_t> & evictedIds);
  void NotifyCallback(pair<size_t, size_t> const & addedIds, pair<size_t, size_t> const & evictedIds);

  size_t const m_maxItemCount;
  string const m_filePath;

  mutable mutex m_dataGuard; // protects data for stealing
  vector<location::GpsInfo> m_points; // accumulated points for adding
  hours m_duration;
  bool m_needClear; // need clear file

  mutex m_callbackGuard;
  // Callback is protected by m_callbackGuard. It ensures that SetCallback and call callback
  // will not be interleaved and after SetCallback(null) callbakc is never called. The negative side
  // is that GpsTrack.SetCallback must be never called from the callback.
  TGpsTrackDiffCallback m_callback;
  bool m_needSendSnapshop; // need send initial snapshot

  unique_ptr<GpsTrackStorage> m_storage; // used in the worker thread
  unique_ptr<GpsTrackCollection> m_collection; // used in the worker thread
  unique_ptr<IGpsTrackFilter> m_filter; // used in the worker thread

  mutex m_threadGuard;
  threads::SimpleThread m_thread;
  bool m_threadExit; // need exit thread
  bool m_threadWakeup; // need wakeup thread
  condition_variable m_cv;
};
