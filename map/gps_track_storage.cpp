#include "map/gps_track_storage.hpp"

#include "coding/internal/file_data.hpp"

#include "std/algorithm.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

namespace
{

// Number of items for batch processing
size_t constexpr kItemBlockSize = 1000;

// TODO
// GpsInfo can be compressed

// Size of point in bytes
size_t const kPointSize = 8 * sizeof(double) + sizeof(uint8_t);

void Pack(char * p, location::GpsInfo const & info)
{  
  memcpy(p + 0 * sizeof(double), &info.m_timestamp, sizeof(double));
  memcpy(p + 1 * sizeof(double), &info.m_latitude, sizeof(double));
  memcpy(p + 2 * sizeof(double), &info.m_longitude, sizeof(double));
  memcpy(p + 3 * sizeof(double), &info.m_altitude, sizeof(double));
  memcpy(p + 4 * sizeof(double), &info.m_speed, sizeof(double));
  memcpy(p + 5 * sizeof(double), &info.m_bearing, sizeof(double));
  memcpy(p + 6 * sizeof(double), &info.m_horizontalAccuracy, sizeof(double));
  memcpy(p + 7 * sizeof(double), &info.m_verticalAccuracy, sizeof(double));
  ASSERT_LESS_OR_EQUAL(static_cast<int>(info.m_source), 255, ());
  uint8_t const source = static_cast<uint8_t>(info.m_source);
  memcpy(p + 8 * sizeof(double), &source, sizeof(uint8_t));
}

void Unpack(char const * p, location::GpsInfo & info)
{
  memcpy(&info.m_timestamp, p + 0 * sizeof(double), sizeof(double));
  memcpy(&info.m_latitude, p + 1 * sizeof(double), sizeof(double));
  memcpy(&info.m_longitude, p + 2 * sizeof(double), sizeof(double));
  memcpy(&info.m_altitude, p + 3 * sizeof(double), sizeof(double));
  memcpy(&info.m_speed, p + 4 * sizeof(double), sizeof(double));
  memcpy(&info.m_bearing, p + 5 * sizeof(double), sizeof(double));
  memcpy(&info.m_horizontalAccuracy, p + 6 * sizeof(double), sizeof(double));
  memcpy(&info.m_verticalAccuracy, p + 7 * sizeof(double), sizeof(double));
  uint8_t source;
  memcpy(&source, p + 8 * sizeof(double), sizeof(uint8_t));
  info.m_source = static_cast<location::TLocationSource>(source);
}

} // namespace

GpsTrackStorage::GpsTrackStorage(string const & filePath, size_t maxItemCount)
  : m_filePath(filePath)
  , m_maxItemCount(maxItemCount)
  , m_itemCount(0)
{
  ASSERT_GREATER(m_maxItemCount, 0, ());

  m_stream = fstream(m_filePath, ios::in|ios::out|ios::binary|ios::ate);

  if (m_stream)
  {
    size_t const fileSize = m_stream.tellp();

    m_itemCount = fileSize / kPointSize;

    // Set write position after last item position
    m_stream.seekp(m_itemCount * kPointSize, ios::beg);
    if (0 != (m_stream.rdstate() & (ios::failbit | ios::badbit)))
      MYTHROW(OpenException, ("File:", m_filePath));
  }
  else
  {
    m_stream = fstream(m_filePath, ios::in|ios::out|ios::binary|ios::trunc);

    if (!m_stream)
      MYTHROW(OpenException, ("File:", m_filePath));

    m_itemCount = 0;
  }
}

void GpsTrackStorage::Append(vector<TItem> const & items)
{
  ASSERT(m_stream.is_open(), ());

  if (items.empty())
    return;

  bool const needTrunc = (m_itemCount + items.size()) > (m_maxItemCount * 2); // see NOTE in declaration

  if (needTrunc)
    TruncFile();

  // Write position must be after last item position
  ASSERT_EQUAL(m_stream.tellp(), m_itemCount * kPointSize, ());

  vector<char> buff(min(kItemBlockSize, items.size()) * kPointSize);
  for (size_t i = 0; i < items.size();)
  {
    size_t const n = min(items.size() - i, kItemBlockSize);

    for (size_t j = 0; j < n; ++j)
      Pack(&buff[0] + j * kPointSize, items[i + j]);

    m_stream.write(&buff[0], n * kPointSize);
    if (0 != (m_stream.rdstate() & (ios::failbit | ios::badbit )))
      MYTHROW(WriteException, ("File:", m_filePath));

    i += n;
  }

  m_stream.flush();
  if (0 != (m_stream.rdstate() & (ios::failbit | ios::badbit )))
    MYTHROW(WriteException, ("File:", m_filePath));

  m_itemCount += items.size();
}

void GpsTrackStorage::Clear()
{
  ASSERT(m_stream.is_open(), ());

  m_itemCount = 0;

  m_stream.close();
  m_stream = fstream(m_filePath, ios::in|ios::out|ios::binary|ios::trunc);

  if (!m_stream)
    MYTHROW(WriteException, ("File:", m_filePath));

  // Write position is set to the begin of file
  ASSERT_EQUAL(m_stream.tellp(), 0, ());
}

void GpsTrackStorage::ForEach(std::function<bool(TItem const & item)> const & fn)
{
  ASSERT(m_stream.is_open(), ());

  size_t i = GetFirstItemIndex();

  // Set read position to the first item
  m_stream.seekg(i * kPointSize, ios::beg);
  if (0 != (m_stream.rdstate() & (ios::failbit | ios::badbit)))
    MYTHROW(ReadException, ("File:", m_filePath));

  vector<char> buff(min(kItemBlockSize, m_itemCount) * kPointSize);
  for (; i < m_itemCount;)
  {
    size_t const n = min(m_itemCount - i, kItemBlockSize);

    m_stream.read(&buff[0], n * kPointSize);
    if (0 != (m_stream.rdstate() & (ios::failbit | ios::badbit | ios::eofbit)))
      MYTHROW(ReadException, ("File:", m_filePath));

    for (size_t j = 0; j < n; ++j)
    {      
      TItem item;
      Unpack(&buff[0] + j * kPointSize, item);
      if (!fn(item))
        return;
    }

    i += n;
  }
}

void GpsTrackStorage::TruncFile()
{
  string const tmpFilePath = m_filePath + ".tmp";

  fstream tmp = fstream(tmpFilePath, ios::in|ios::out|ios::binary|ios::trunc);
  if (!tmp)
    MYTHROW(WriteException, ("Unable to create temporary file:", m_filePath));

  size_t i = GetFirstItemIndex();

  // Set read position to the first item
  m_stream.seekg(i * kPointSize, ios::beg);
  if (0 != (m_stream.rdstate() & (ios::failbit | ios::badbit)))
    MYTHROW(ReadException, ("File:", m_filePath));

  size_t newItemCount = 0;

  // Copy items
  vector<char> buff(min(kItemBlockSize, m_itemCount) * kPointSize);
  for (; i < m_itemCount;)
  {
    size_t const n = min(m_itemCount - i, kItemBlockSize);

    m_stream.read(&buff[0], n * kPointSize);
    if (0 != (m_stream.rdstate() & (ios::failbit | ios::badbit | ios::eofbit)))
      MYTHROW(ReadException, ("File:", m_filePath));

    tmp.write(&buff[0], n * kPointSize);
    if (0 != (tmp.rdstate() & (ios::failbit | ios::badbit)))
      MYTHROW(WriteException, ("File:", tmpFilePath));

    i += n;
    newItemCount += n;
  }
  buff.clear();
  buff.shrink_to_fit();

  tmp.close();
  m_stream.close();

  // Replace file
  if (!my::DeleteFileX(m_filePath) ||
      !my::RenameFileX(tmpFilePath, m_filePath))
  {
    MYTHROW(WriteException, ("File:", m_filePath));
  }

  // Reopen stream
  m_stream = fstream(m_filePath, ios::in|ios::out|ios::binary|ios::ate);
  if (!m_stream)
    MYTHROW(WriteException, ("File:", m_filePath));

  m_itemCount = newItemCount;

  // Write position must be after last item position (end of file)
  ASSERT_EQUAL(m_stream.tellp(), m_itemCount * kPointSize, ());
}

size_t GpsTrackStorage::GetFirstItemIndex() const
{
  return (m_itemCount > m_maxItemCount) ? (m_itemCount - m_maxItemCount) : 0; // see NOTE in declaration
}
