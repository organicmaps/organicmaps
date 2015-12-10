#pragma once

#include "platform/location.hpp"

#include "base/exception.hpp"

#include "std/fstream.hpp"
#include "std/function.hpp"
#include "std/limits.hpp"
#include "std/string.hpp"

class GpsTrackFile final
{
public:
  DECLARE_EXCEPTION(WriteFileException, RootException);
  DECLARE_EXCEPTION(ReadFileException, RootException);

  using TItem = location::GpsTrackInfo;

  GpsTrackFile();
  ~GpsTrackFile();

  /// Opens file with track data.
  /// @param filePath - path to the file on disk
  /// @param maxItemCount - max number of items in recycling file
  /// @return If file does not exist then returns false, if everything is ok then returns true.
  /// @exception WriteFileException if seek fails.
  bool Open(string const & filePath, size_t maxItemCount);

  /// Creates new file
  /// @param filePath - path to the file on disk
  /// @param maxItemCount - max number of items in recycling file
  /// @return If file cannot be created then returns false, if everything is ok then returns true.
  bool Create(string const & filePath, size_t maxItemCount);

  /// Returns true if file is open, otherwise returns false
  bool IsOpen() const;

  /// Flushes all changes and closes file
  /// @exception WriteFileException if write fails
  void Close();

  /// Flushes all changes in file
  /// @exception WriteFileException if write fails
  void Flush();

  /// Appends new point in the file
  /// @param items - collection of gps track points.
  /// @exceptions WriteFileException if write fails.
  void Append(vector<TItem> const & items);

  /// Removes all data from file
  /// @exceptions WriteFileException if write fails.
  void Clear();

  /// Reads file and calls functor for each item with timestamp not earlier than specified
  /// @param fn - callable function, return true to stop ForEach
  /// @exceptions ReadFileException if read fails.
  void ForEach(std::function<bool(TItem const & item)> const & fn);

private:
  void TruncFile();
  size_t GetFirstItemIndex() const;

  string m_filePath;
  fstream m_stream;
  size_t m_maxItemCount; // max count valid items in file, read note
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
