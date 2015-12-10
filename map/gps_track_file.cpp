#include "map/gps_track_file.hpp"

#include "coding/internal/file_data.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

GpsTrackFile::GpsTrackFile()
  : m_maxItemCount(0)
  , m_itemCount(0)
{
}

GpsTrackFile::~GpsTrackFile()
{
}

bool GpsTrackFile::Open(string const & filePath, size_t maxItemCount)
{
  ASSERT(!m_stream.is_open(), ());
  ASSERT(maxItemCount > 0, ());

  m_stream = fstream(filePath, ios::in|ios::out|ios::binary|ios::ate);

  if (!m_stream)
    return false;

  m_filePath = filePath;

  size_t const fileSize = m_stream.tellp();

  m_itemCount = fileSize / sizeof(TItem);
  m_maxItemCount = maxItemCount;

  // Set write position after last item position
  m_stream.seekp(m_itemCount * sizeof(TItem), ios::beg);
  if (0 != (m_stream.rdstate() & (ios::failbit | ios::badbit)))
    MYTHROW(WriteFileException, ("File:", m_filePath));

  return true;
}

bool GpsTrackFile::Create(string const & filePath, size_t maxItemCount)
{
  ASSERT(!m_stream.is_open(), ());
  ASSERT(maxItemCount > 0, ());

  m_stream = fstream(filePath, ios::in|ios::out|ios::binary|ios::trunc);

  if (!m_stream)
    return false;

  m_filePath = filePath;

  m_itemCount = 0;
  m_maxItemCount = maxItemCount;

  // Write position is set to the begin of file
  ASSERT_EQUAL(m_stream.tellp(), 0, ());

  return true;
}

bool GpsTrackFile::IsOpen() const
{
  return m_stream.is_open();
}

void GpsTrackFile::Close()
{
  ASSERT(m_stream.is_open(), ());

  m_stream.close();
  if (0 != (m_stream.rdstate() & (ios::failbit | ios::badbit)))
    MYTHROW(WriteFileException, ("File:", m_filePath));

  m_filePath.clear();
  m_itemCount = 0;
  m_maxItemCount = 0;
}

void GpsTrackFile::Flush()
{
  ASSERT(m_stream.is_open(), ());

  m_stream.flush();
  if (0 != (m_stream.rdstate() & (ios::failbit | ios::badbit)))
    MYTHROW(WriteFileException, ("File:", m_filePath));
}

void GpsTrackFile::Append(vector<TItem> const & items)
{
  ASSERT(m_stream.is_open(), ());

  bool const needTrunc = (m_itemCount + items.size()) > (m_maxItemCount * 2); // see NOTE in class declaration

  if (needTrunc)
    TruncFile();

  // Write position must be after last item position
  ASSERT_EQUAL(m_stream.tellp(), m_itemCount * sizeof(TItem), ());

  m_stream.write(reinterpret_cast<const char *>(&items[0]), items.size() * sizeof(TItem));
  if (0 != (m_stream.rdstate() & (ios::failbit | ios::badbit )))
    MYTHROW(WriteFileException, ("File:", m_filePath));

  m_itemCount += items.size();
}

void GpsTrackFile::Clear()
{
  m_itemCount = 0;

  m_stream.close();
  m_stream = fstream(m_filePath, ios::in|ios::out|ios::binary|ios::trunc);

  if (!m_stream)
    MYTHROW(WriteFileException, ("File:", m_filePath));

  // Write position is set to the begin of file
  ASSERT_EQUAL(m_stream.tellp(), 0, ());
}

void GpsTrackFile::ForEach(double timestampSince, std::function<bool(TItem const & item)> const & fn)
{
  ASSERT(m_stream.is_open(), ());

  size_t i = GetFirstItemIndex();

  // Set read position to the first item
  m_stream.seekg(i * sizeof(TItem), ios::beg);
  if (0 != (m_stream.rdstate() & (ios::failbit | ios::badbit)))
    MYTHROW(ReadFileException, ("File:", m_filePath));

  for (; i < m_itemCount; ++i)
  {
    TItem item;
    m_stream.read(reinterpret_cast<char *>(&item), sizeof(TItem));
    if (0 != (m_stream.rdstate() & (ios::failbit | ios::badbit | ios::eofbit)))
      MYTHROW(ReadFileException, ("File:", m_filePath));

    if (item.m_timestamp >= timestampSince)
    {
      if (!fn(item))
        break;
    }
  }
}

void GpsTrackFile::TruncFile()
{
  string const tmpFilePath = m_filePath + ".tmp";

  fstream tmp = fstream(tmpFilePath, ios::in|ios::out|ios::binary|ios::trunc);
  if (!tmp)
    MYTHROW(WriteFileException, ("File:", m_filePath));

  size_t i = GetFirstItemIndex();

  // Set read position to the first item
  m_stream.seekg(i * sizeof(TItem), ios::beg);
  if (0 != (m_stream.rdstate() & (ios::failbit | ios::badbit)))
    MYTHROW(ReadFileException, ("File:", m_filePath));

  size_t itemCount = 0;
  for (; i < m_itemCount; ++i, ++itemCount)
  {
    TItem item;
    m_stream.read(reinterpret_cast<char *>(&item), sizeof(TItem));
    if (0 != (m_stream.rdstate() & (ios::failbit | ios::badbit | ios::eofbit)))
      MYTHROW(ReadFileException, ("File:", m_filePath));

    tmp.write(reinterpret_cast<char *>(&item), sizeof(TItem));
    if (0 != (tmp.rdstate() & (ios::failbit | ios::badbit)))
      MYTHROW(WriteFileException, ("File:", tmpFilePath));
  }

  tmp.close();
  m_stream.close();

  if (!my::DeleteFileX(m_filePath) ||
      !my::RenameFileX(tmpFilePath, m_filePath))
  {
    MYTHROW(WriteFileException, ("File:", m_filePath));
  }

  m_stream = fstream(m_filePath, ios::in|ios::out|ios::binary|ios::ate);
  if (!m_stream)
    MYTHROW(WriteFileException, ("File:", m_filePath));

  m_itemCount = itemCount;

  // Write position must be after last item position (end of file)
  ASSERT_EQUAL(m_stream.tellp(), m_itemCount * sizeof(TItem), ());
}

size_t GpsTrackFile::GetFirstItemIndex() const
{
  return (m_itemCount > m_maxItemCount) ? (m_itemCount - m_maxItemCount) : 0; // see NOTE in class declaration
}
