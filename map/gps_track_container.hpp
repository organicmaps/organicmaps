#pragma once

#include "drape_frontend/gps_track_point.hpp"

#include "std/chrono.hpp"
#include "std/deque.hpp"
#include "std/function.hpp"
#include "std/mutex.hpp"

class GpsTrackContainer final
{
public:

  static uint32_t constexpr kInvalidId = numeric_limits<uint32_t>::max();

  /// Notification callback about a change of the gps track.
  /// @param toAdd - collection of points to add.
  /// @param toRemove - collection of point indices to remove.
  /// @note Calling of a GpsTrackContainer's function from the callback causes deadlock.
  using TGpsTrackDiffCallback = std::function<void(vector<df::GpsTrackPoint> && toAdd, vector<uint32_t> && toRemove)>;

  GpsTrackContainer();

  /// Sets track duration in hours.
  /// @note Callback is called with 'toRemove' points, if some points were removed.
  /// By default, duration is 24h.
  void SetDuration(hours duration);

  /// Returns track duraion in hours
  hours GetDuration() const;

  /// Sets max number of points in the track.
  /// @note Callback is called with 'toRemove' points, if some points were removed.
  /// By default, max size is 100k.
  void SetMaxSize(size_t maxSize);

  /// Returns max number of points in the track
  size_t GetMaxSize() const;

  /// Sets callback on change of gps track.
  /// @param callback - callback callable object
  /// @param sendAll - If it is true then callback is called with all points as 'toAdd' points, if there are points.
  /// If helps to avoid race condition and complex synchronizations if we called getter GetGpsTrackPoints
  /// and waited for notification OnGpsTrackChanged. If it is false, then there is no ant callback and client is responsible
  /// for getting of initial state and syncing it with callbacks.
  /// @note Only one callback is supported at time.
  void SetCallback(TGpsTrackDiffCallback callback, bool sendAll);

  /// Adds new point in the track.
  /// @param point - new point in the Mercator projection.
  /// @param speedMPS - current speed in the new point in M/S.
  /// @param timestamp - timestamp of the point.
  /// @note Callback is called with 'toAdd' and 'toRemove' points.
  /// @returns the point unique identifier or kInvalidId if point has incorrect time.
  uint32_t AddPoint(m2::PointD const & point, double speedMPS, double timestamp);

  /// Returns points snapshot from the container.
  /// @param points - output for collection of points.
  void GetPoints(vector<df::GpsTrackPoint> & points) const;

  /// Clears collection of point in the track.
  /// @note Callback is called with 'toRemove' points, if need.
  void Clear();

private:
  void RemoveOldPoints(vector<uint32_t> & removedIds);
  void CopyPoints(vector<df::GpsTrackPoint> & points) const;

  mutable mutex m_guard;

  TGpsTrackDiffCallback m_callback;

  // Max duration of track, by default is 24h.
  hours m_trackDuration;

  // Max number of points in track, by default is 100k.
  size_t m_maxSize;

  // Collection of points, by nature is asc. sorted by m_timestamp.
  // Max size of m_points is adjusted by m_trackDuration and m_maxSize.
  deque<df::GpsTrackPoint> m_points;

  // Simple counter which is used to generate point unique ids.
  uint32_t m_counter;
};
