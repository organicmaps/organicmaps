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
  DECLARE_EXCEPTION(CorruptedFileException, RootException);

  /// Invalid identifier for point
  static size_t const kInvalidId; // = numeric_limits<size_t>::max();

  using TItem = location::GpsTrackInfo;

  GpsTrackFile();
  ~GpsTrackFile();

  /// Opens file with track data.
  /// @param filePath - path to the file on disk
  /// @param maxItemCount - max number of items in recycling file
  /// @return If file does not exist then returns false, if everything is ok then returns true.
  /// @exception If file corrupted then throws CorruptedFileException file, or ReadFileException if read fails.
  bool Open(string const & filePath, size_t maxItemCount);

  /// Creates new file
  /// @param filePath - path to the file on disk
  /// @param maxItemCount - max number of items in recycling file
  /// @return If file cannot be created then false is returned, if everything is ok then returns true.
  /// @exceptions WriteFileException if write fails.
  bool Create(string const & filePath, size_t maxItemCount);

  /// Returns true if file is open, otherwise returns false
  bool IsOpen() const;

  /// Flushes all changes and closes file
  /// @exceptions WriteFileException if write fails.
  void Close();

  /// Flushes all changes in file
  /// @exceptions WriteFileException if write fails.
  void Flush();

  /// Appends new point in the file
  /// @param item - gps track point.
  /// @param evictedId - identifier of evicted point due to recycling, kInvalidId if there is no evicted point.
  /// @returns identifier of point, kInvalidId if point was not added.
  /// @note Timestamp must be not less than GetTimestamp(), otherwise function returns false.
  /// @note File is circular, when GetMaxItemCount() limit is reached, old point is evicted from file.
  /// @exceptions WriteFileException if write fails.
  size_t Append(TItem const & item, size_t & evictedId);

  /// Remove all points from the file
  /// @returns range of identifiers of evicted points, or pair(kInvalidId,kInvalidId) if nothing was evicted.
  /// @exceptions WriteFileException if write fails.
  pair<size_t, size_t> Clear();

  /// Returns max number of points in recycling file
  size_t GetMaxCount() const;

  /// Returns number of items in the file, this values is <= GetMaxItemCount()
  size_t GetCount() const;

  /// Returns true if file does not contain points
  bool IsEmpty() const;

  /// Returns upper bound timestamp, or zero if there is no points
  double GetTimestamp() const;

  /// Enumerates all points from the file in timestamp ascending order
  /// @param fn - callable object which receives points. If fn returns false then enumeration is stopped.
  /// @exceptions CorruptedFileException if file is corrupted or ReadFileException if read fails
  void ForEach(function<bool(TItem const & item, size_t id)> const & fn);

  /// Drops points earlier than specified date
  /// @param timestamp - timestamp of lower bound, number of seconds since 1.1.1970.
  /// @returns range of identifiers of evicted points, or pair(kInvalidId,kInvalidId) if nothing was evicted.
  /// @exceptions CorruptedFileException if file is corrupted, ReadFileException if read fails
  /// or WriteFileException if write fails.
  pair<size_t, size_t> DropEarlierThan(double timestamp);

private:
  /// Header, stored in beginning of file
  /// @note Due to recycling, indexes of items are reused, but idendifiers are unique
  /// until Clear or create new file
  struct Header
  {
    size_t m_maxItemCount; // max number of items in recycling file (n + 1 for end element)
    double m_timestamp; // upper bound timestamp
    size_t m_first; // index of first item
    size_t m_last; // index of last item, items are in range [first,last)
    size_t m_lastId; // identifier of the last item

    Header()
      : m_maxItemCount(0)
      , m_timestamp(0)
      , m_first(0)
      , m_last(0)
      , m_lastId(0)
    {}
  };

  void LazyWriteHeader();

  bool ReadHeader(Header & header);
  void WriteHeader(Header const & header);

  bool ReadItem(size_t index, TItem & item);
  void WriteItem(size_t index, TItem const & item);

  static size_t ItemOffset(size_t index);
  static size_t Distance(size_t first, size_t last, size_t maxItemCount);

  string m_filePath; // file name
  fstream m_stream; // buffered file stream
  Header m_header; // file header

  uint32_t m_lazyWriteHeaderCount; // request count for write header
};
