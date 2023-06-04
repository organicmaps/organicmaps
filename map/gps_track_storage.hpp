#pragma once

#include "platform/location.hpp"

#include "base/exception.hpp"
#include "base/macros.hpp"

#include <fstream>
#include <functional>
#include <string>
#include <vector>

class GpsTrackStorage final
{
public:
  DECLARE_EXCEPTION(OpenException, RootException);
  DECLARE_EXCEPTION(WriteException, RootException);
  DECLARE_EXCEPTION(ReadException, RootException);

  using TItem = location::GpsInfo;

  /// Opens storage with track data.
  /// @param filePath - path to the file on disk
  /// @param maxItemCount - max number of items in recycling file
  /// @exception OpenException if seek fails.
  GpsTrackStorage(std::string const & filePath, size_t maxItemCount);

  /// Appends new point to the storage
  /// @param items - collection of gps track points.
  /// @exceptions WriteException if write fails or ReadException if read fails.
  void Append(std::vector<TItem> const & items);

  /// Removes all data from the storage
  /// @exceptions WriteException if write fails.
  void Clear();

  /// Reads the storage and calls functor for each item
  /// @param fn - callable function, return true to stop ForEach
  /// @exceptions ReadException if read fails.
  void ForEach(std::function<bool(TItem const & item)> const & fn);

private:
  DISALLOW_COPY_AND_MOVE(GpsTrackStorage);

  void TruncFile();
  size_t GetFirstItemIndex() const;

  std::string const m_filePath;
  size_t const m_maxItemCount;
  std::fstream m_stream;
  size_t m_itemCount; // current number of items in file, read note

  // NOTE
  // New items append to the end of file, when file become too big, it is truncated.
  // Here 'silly window sindrome' is possible: for example max file size is 100k items,
  // and 99k items are filled, when 2k items is coming, then 98k items is copying to the tmp file
  // which replaces origin file, and 2k added to the new file. Next time, when 1k items
  // is comming, then again file truncated. That means that trunc will happens every time.
  // Similar problem in TCP called 'silly window sindrome'.
  // To prevent that problem, max file size can be 2 x m_maxItemCount items, when current number of items m_itemCount
  // exceed 2 x m_maxItemCount, then second half of file - m_maxItemCount items is copying to the tmp file,
  // which replaces origin file. That means that trunc will happens only then new m_maxItemCount items will be
  // added but not every time.
};
