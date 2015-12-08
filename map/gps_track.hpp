#pragma once

#include "map/gps_track_collection.hpp"
#include "map/gps_track_file.hpp"

#include "std/mutex.hpp"
#include "std/unique_ptr.hpp"

class GpsTrack final
{
public:
  static size_t const kInvalidId; // = numeric_limits<size_t>::max();

  GpsTrack(string const & filePath);

  /// Adds point or collection of points to gps tracking
  void AddPoint(location::GpsTrackInfo const & point);
  void AddPoints(vector<location::GpsTrackInfo> const & points);

  /// Clears any previous tracking info
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
  /// @note Calling of a GpsTrack's function from the callback causes deadlock.
  using TGpsTrackDiffCallback = std::function<void(vector<pair<size_t, location::GpsTrackInfo>> && toAdd,
                                                   pair<size_t, size_t> const & toRemove)>;

  /// Sets callback on change of gps track.
  /// @param callback - callback callable object
  /// @note Only one callback is supported at time.
  /// @note When sink is attached, it receives all points in 'toAdd' at first time,
  /// next time callbacks it receives only modifications. It simplifies getter/callback model.
  void SetCallback(TGpsTrackDiffCallback callback);

private:
  void LazyInitFile();
  void LazyInitCollection();
  void SendInitialSnapshot();

  string const m_filePath;

  mutable mutex m_guard;

  hours m_duration;

  unique_ptr<GpsTrackFile> m_file;

  unique_ptr<GpsTrackCollection> m_collection;

  TGpsTrackDiffCallback m_callback;
};

GpsTrack & GetDefaultGpsTrack();
