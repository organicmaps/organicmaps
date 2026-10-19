#pragma once

#include "platform/location.hpp"

#include "base/macros.hpp"

#include <string>
#include <vector>

class GpsTrackStorage final
{
public:
  using TItem = location::GpsInfo;

  explicit GpsTrackStorage(std::string const & filePath);

  /// Restores complete points from an existing backup without creating a file.
  /// Logs errors and discards an unusable backup, returning no points.
  /// Must be called before appending to an existing backup.
  std::vector<TItem> Load();

  /// Appends a batch of points to the backup.
  /// @param items - collection of gps track points.
  /// @returns false on an I/O error; the batch can be retried.
  /// @note A partially written batch must be retried in full.
  bool Append(std::vector<TItem> const & items);

  /// Deletes the backup file.
  void Clear();

private:
  DISALLOW_COPY_AND_MOVE(GpsTrackStorage);

  std::string const m_filePath;
  size_t m_itemCount = 0;
};
