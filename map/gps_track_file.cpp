#include "map/gps_track_file.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "std/algorithm.hpp"

namespace
{

size_t constexpr kLinearSearchDistance = 10;
size_t constexpr kLazyWriteHeaderMaxCount = 100;

} // namespace

size_t const GpsTrackFile::kInvalidId = numeric_limits<size_t>::max();

GpsTrackFile::GpsTrackFile()
  : m_lazyWriteHeaderCount(0)
{
}

GpsTrackFile::~GpsTrackFile()
{
  ASSERT(!m_stream.is_open(), ("Don't forget to close file"));

  if (m_stream.is_open())
  {
    try
    {
      Close();
    }
    catch (RootException const & e)
    {
      LOG(LINFO, ("Close caused exception", e.Msg()));
    }
  }
}

bool GpsTrackFile::Open(string const & filePath, size_t maxItemCount)
{
  ASSERT(!m_stream.is_open(), ("File must not be open on OpenFile"));
  ASSERT(maxItemCount > 0, ());

  m_stream = fstream(filePath, ios::in|ios::out|ios::binary);

  if (!m_stream)
    return false;

  try
  {
    m_filePath = filePath;

    // Check file integrity:
    // - file size if correct;
    // - Header fields m_first, m_last, m_lastId and m_timestamp are correct;
    // - front and back items are correct, if exist.

    m_stream.seekg(0, ios::end);
    size_t const fileSize = m_stream.tellg();
    m_stream.seekg(0, ios::beg);

    if (fileSize < sizeof(Header))
      MYTHROW(CorruptedFileException, ("File:", m_filePath));

    if ((fileSize - sizeof(Header)) % sizeof(TItem) != 0)
      MYTHROW(CorruptedFileException, ("File:", m_filePath));

    size_t const itemCount = (fileSize - sizeof(Header)) / sizeof(TItem);

    if (!ReadHeader(m_header))
      MYTHROW(CorruptedFileException, ("File:", m_filePath));

    if (m_header.m_maxItemCount != (1 + maxItemCount))
      MYTHROW(CorruptedFileException, ("File:", m_filePath));

    if (itemCount > m_header.m_maxItemCount)
      MYTHROW(CorruptedFileException, ("File:", m_filePath));

    if (itemCount == 0)
    {
      if (m_header.m_first != 0 && m_header.m_last != 0)
        MYTHROW(CorruptedFileException, ("File:", m_filePath));
    }
    else
    {
      if (m_header.m_first >= itemCount || m_header.m_last > itemCount)
        MYTHROW(CorruptedFileException, ("File:", m_filePath));
    }

    if (m_header.m_lastId < itemCount)
      MYTHROW(CorruptedFileException, ("File:", m_filePath));

    if (m_header.m_first != m_header.m_last)
    {
      TItem frontItem;
      if (!ReadItem(m_header.m_first, frontItem))
        MYTHROW(CorruptedFileException, ("File:", m_filePath));

      TItem backItem;
      size_t backIndex = (m_header.m_first + Distance(m_header.m_first, m_header.m_last, m_header.m_maxItemCount) - 1) % m_header.m_maxItemCount;
      if (!ReadItem(backIndex, backItem))
        MYTHROW(CorruptedFileException, ("File:", m_filePath));

      if (frontItem.m_timestamp > backItem.m_timestamp || m_header.m_timestamp != backItem.m_timestamp)
        MYTHROW(CorruptedFileException, ("File:", m_filePath));
    }
  }
  catch (RootException &)
  {
    m_header = Header();
    m_filePath.clear();
    m_stream.close();
    throw;
  }

  return true;
}

bool GpsTrackFile::Create(string const & filePath, size_t maxItemCount)
{
  ASSERT(!m_stream.is_open(), ("File must not be open on CreateFile"));
  ASSERT(maxItemCount > 0, ());

  m_stream = fstream(filePath, ios::in|ios::out|ios::binary|ios::trunc);

  if (!m_stream)
    return false;

  try
  {
    m_filePath = filePath;

    m_header = Header();
    m_header.m_maxItemCount = maxItemCount + 1;

    WriteHeader(m_header);
  }
  catch (RootException &)
  {
    m_header = Header();
    m_filePath.clear();
    m_stream.close();
    throw;
  }

  return true;
}

bool GpsTrackFile::IsOpen() const
{
  return m_stream.is_open();
}

void GpsTrackFile::Close()
{
  ASSERT(m_stream.is_open(), ("File is not open"));

  if (m_lazyWriteHeaderCount != 0)
  {
    m_lazyWriteHeaderCount = 0;
    WriteHeader(m_header);
  }

  m_stream.close();
  if (0 != (m_stream.rdstate() & (ios::failbit | ios::badbit)))
    MYTHROW(WriteFileException, ("File:", m_filePath));

  m_header = Header();
  m_filePath.clear();
}

void GpsTrackFile::Flush()
{
  ASSERT(m_stream.is_open(), ("File is not open"));

  if (m_lazyWriteHeaderCount != 0)
  {
    m_lazyWriteHeaderCount = 0;
    WriteHeader(m_header);
  }

  m_stream.flush();
  if (0 != (m_stream.rdstate() & (ios::failbit | ios::badbit)))
    MYTHROW(WriteFileException, ("File:", m_filePath));
}

size_t GpsTrackFile::Append(TItem const & item, size_t & evictedId)
{
  ASSERT(m_stream.is_open(), ("File is not open"));

  if (item.m_timestamp < m_header.m_timestamp)
  {
    evictedId = kInvalidId; // nothing was evicted
    return kInvalidId; // nothing was added
  }

  size_t const newLast = (m_header.m_last + 1) % m_header.m_maxItemCount;
  size_t const newFirst = (newLast == m_header.m_first) ? ((m_header.m_first + 1) % m_header.m_maxItemCount) : m_header.m_first;

  WriteItem(m_header.m_last, item);

  size_t const addedId = m_header.m_lastId;

  if (m_header.m_first == newFirst)
    evictedId = kInvalidId; // nothing was evicted
  else
    evictedId = m_header.m_lastId - GetCount(); // the id of the first item

  m_header.m_first = newFirst;
  m_header.m_last = newLast;
  m_header.m_timestamp = item.m_timestamp;
  m_header.m_lastId += 1;

  LazyWriteHeader();

  return addedId;
}

void GpsTrackFile::ForEach(function<bool(TItem const & item, size_t id)> const & fn)
{
  ASSERT(m_stream.is_open(), ("File is not open"));

  double prevTimestamp = 0;

  size_t id = m_header.m_lastId - GetCount(); // the id of the first item

  for (size_t i = m_header.m_first; i != m_header.m_last; i = (i + 1) % m_header.m_maxItemCount)
  {
    TItem item;
    if (!ReadItem(i, item))
      MYTHROW(CorruptedFileException, ("File:", m_filePath));

    if (prevTimestamp > item.m_timestamp)
      MYTHROW(CorruptedFileException, ("File:", m_filePath));

    size_t itemId = id;
    if (!fn(item, itemId))
      break;

    prevTimestamp = item.m_timestamp;
    ++id;
  }
}

pair<size_t, size_t> GpsTrackFile::DropEarlierThan(double timestamp)
{
  ASSERT(m_stream.is_open(), ("File is not open"));

  if (IsEmpty())
    return make_pair(kInvalidId, kInvalidId); // nothing was dropped

  if (m_header.m_timestamp <= timestamp)
    return Clear();

  size_t const n = GetCount();

  ASSERT_GREATER_OR_EQUAL(m_header.m_lastId, n, ());

  // Try linear search for short distance
  // In common case elements will be removed from the tail by small pieces
  size_t const linearSearchCount = min(kLinearSearchDistance, n);
  for (size_t i = m_header.m_first, j = 0; i != m_header.m_last; i = (i + 1) % m_header.m_maxItemCount, ++j)
  {
    if (j >= linearSearchCount)
      break;

    TItem item;
    if (!ReadItem(i, item))
      MYTHROW(CorruptedFileException, ("File:", m_filePath));

    if (item.m_timestamp >= timestamp)
    {
      // Dropped range is
      pair<size_t, size_t> const res = make_pair(m_header.m_lastId - n,
                                                 m_header.m_lastId - n + j - 1);

      // Update header.first, if need
      if (i != m_header.m_first)
      {        
        m_header.m_first = i;

        LazyWriteHeader();
      }

      return res;
    }
  }

  // By nature items are sorted by timestamp.
  // Use lower_bound algorithm to find first element later than timestamp.
  size_t len = n, first = m_header.m_first;
  while (len > 0)
  {
    size_t const step = len / 2;
    size_t const index = (first + step) % m_header.m_maxItemCount;

    TItem item;
    if (!ReadItem(index, item))
      MYTHROW(CorruptedFileException, ("File:", m_filePath));

    if (item.m_timestamp < timestamp)
    {
      first = (index + 1) % m_header.m_maxItemCount;
      len -= step + 1;
    }
    else
      len = step;
  }

  // Dropped range is
  pair<size_t, size_t> const res =
          make_pair(m_header.m_lastId - n,
                    m_header.m_lastId - n + Distance(m_header.m_first, first, m_header.m_maxItemCount) - 1);

  // Update header.first, if need
  if (first != m_header.m_first)
  {
    m_header.m_first = first;

    LazyWriteHeader();
  }

  return res;
}

pair<size_t, size_t> GpsTrackFile::Clear()
{
  ASSERT(m_stream.is_open(), ("File is not open"));

  if (m_header.m_first == 0 && m_header.m_last == 0 &&
      m_header.m_lastId == 0 && m_header.m_timestamp == 0)
  {
     return make_pair(kInvalidId, kInvalidId); // nothing was dropped
  }

  // Dropped range is
  pair<size_t, size_t> const res = make_pair(m_header.m_lastId - GetCount(),
                                             m_header.m_lastId - 1);

  m_header.m_first = 0;
  m_header.m_last = 0;
  m_header.m_lastId = 0;
  m_header.m_timestamp = 0;

  LazyWriteHeader();

  return res;
}

size_t GpsTrackFile::GetMaxCount() const
{
  return m_header.m_maxItemCount == 0 ? 0 : m_header.m_maxItemCount - 1;
}

size_t GpsTrackFile::GetCount() const
{
  return Distance(m_header.m_first, m_header.m_last, m_header.m_maxItemCount);
}

bool GpsTrackFile::IsEmpty() const
{
  return m_header.m_first == m_header.m_last;
}

double GpsTrackFile::GetTimestamp() const
{
  return m_header.m_timestamp;
}

void GpsTrackFile::LazyWriteHeader()
{
  ++m_lazyWriteHeaderCount;
  if (m_lazyWriteHeaderCount < kLazyWriteHeaderMaxCount)
    return;

  m_lazyWriteHeaderCount = 0;
  WriteHeader(m_header);
}

bool GpsTrackFile::ReadHeader(Header & header)
{
  m_stream.seekg(0, ios::beg);
  m_stream.read(reinterpret_cast<char *>(&header), sizeof(header));
  if (0 != (m_stream.rdstate() & (ios::failbit | ios::badbit)))
    MYTHROW(ReadFileException, ("File:", m_filePath));
  return ((m_stream.rdstate() & ios::eofbit) == 0);
}

void GpsTrackFile::WriteHeader(Header const & header)
{
  m_stream.seekp(0, ios::beg);
  m_stream.write(reinterpret_cast<char const *>(&header), sizeof(header));
  if (0 != (m_stream.rdstate() & (ios::failbit | ios::badbit)))
    MYTHROW(WriteFileException, ("File:", m_filePath));
}

bool GpsTrackFile::ReadItem(size_t index, TItem & item)
{
  size_t const offset = ItemOffset(index);
  m_stream.seekg(offset, ios::beg);
  m_stream.read(reinterpret_cast<char *>(&item), sizeof(item));
  if (0 != (m_stream.rdstate() & (ios::failbit | ios::badbit)))
    MYTHROW(ReadFileException, ("File:", m_filePath));
  return ((m_stream.rdstate() & ios::eofbit) == 0);
}

void GpsTrackFile::WriteItem(size_t index, TItem const & item)
{
  size_t const offset = ItemOffset(index);
  m_stream.seekp(offset, ios::beg);
  m_stream.write(reinterpret_cast<char const *>(&item), sizeof(item));
  if (0 != (m_stream.rdstate() & (ios::failbit | ios::badbit)))
    MYTHROW(WriteFileException, ("File:", m_filePath));
}

size_t GpsTrackFile::ItemOffset(size_t index)
{
  return sizeof(Header) + index * sizeof(TItem);
}

size_t GpsTrackFile::Distance(size_t first, size_t last, size_t maxItemCount)
{
  return (first <= last) ? (last - first) : (last + maxItemCount - first);
}
