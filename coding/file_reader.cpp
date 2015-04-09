#include "coding/file_reader.hpp"
#include "coding/reader_cache.hpp"
#include "coding/internal/file_data.hpp"

#ifndef LOG_FILE_READER_STATS
#define LOG_FILE_READER_STATS 0
#endif // LOG_FILE_READER_STATS

#if LOG_FILE_READER_STATS && !defined(LOG_FILE_READER_EVERY_N_READS_MASK)
#define LOG_FILE_READER_EVERY_N_READS_MASK 0xFFFFFFFF
#endif

namespace
{
  class FileDataWithCachedSize : public my::FileData
  {
      typedef my::FileData base_t;

  public:
    explicit FileDataWithCachedSize(string const & fileName)
      : base_t(fileName, FileData::OP_READ), m_Size(FileData::Size()) {}

    uint64_t Size() const { return m_Size; }

  private:
    uint64_t m_Size;
  };
}

class FileReader::FileReaderData
{
public:
  FileReaderData(string const & fileName, uint32_t logPageSize, uint32_t logPageCount)
    : m_FileData(fileName), m_ReaderCache(logPageSize, logPageCount)
  {
#if LOG_FILE_READER_STATS
    m_ReadCallCount = 0;
#endif
  }

  ~FileReaderData()
  {
#if LOG_FILE_READER_STATS
    LOG(LINFO, ("FileReader", GetName(), m_ReaderCache.GetStatsStr()));
#endif
  }

  uint64_t Size() const { return m_FileData.Size(); }

  void Read(uint64_t pos, void * p, size_t size)
  {
#if LOG_FILE_READER_STATS
    if (((++m_ReadCallCount) & LOG_FILE_READER_EVERY_N_READS_MASK) == 0)
    {
      LOG(LINFO, ("FileReader", GetName(), m_ReaderCache.GetStatsStr()));
    }
#endif

    return m_ReaderCache.Read(m_FileData, pos, p, size);
  }

private:
  FileDataWithCachedSize m_FileData;
  ReaderCache<FileDataWithCachedSize, LOG_FILE_READER_STATS> m_ReaderCache;

#if LOG_FILE_READER_STATS
  uint32_t m_ReadCallCount;
#endif
};

FileReader::FileReader(string const & fileName, uint32_t logPageSize, uint32_t logPageCount)
  : base_type(fileName), m_pFileData(new FileReaderData(fileName, logPageSize, logPageCount)),
  m_Offset(0), m_Size(m_pFileData->Size())
{
}

FileReader::FileReader(FileReader const & reader, uint64_t offset, uint64_t size)
  : base_type(reader.GetName()), m_pFileData(reader.m_pFileData), m_Offset(offset), m_Size(size)
{
}

uint64_t FileReader::Size() const
{
  return m_Size;
}

void FileReader::Read(uint64_t pos, void * p, size_t size) const
{
  ASSERT ( AssertPosAndSize(pos, size), () );
  m_pFileData->Read(m_Offset + pos, p, size);
}

FileReader FileReader::SubReader(uint64_t pos, uint64_t size) const
{
  ASSERT ( AssertPosAndSize(pos, size), () );
  return FileReader(*this, m_Offset + pos, size);
}

FileReader * FileReader::CreateSubReader(uint64_t pos, uint64_t size) const
{
  ASSERT ( AssertPosAndSize(pos, size), () );
  return new FileReader(*this, m_Offset + pos, size);
}

bool FileReader::AssertPosAndSize(uint64_t pos, uint64_t size) const
{
  uint64_t const allSize1 = Size();
  bool const ret1 = (pos + size <= allSize1);
  ASSERT ( ret1, (pos, size, allSize1) );

  uint64_t const allSize2 = m_pFileData->Size();
  bool const ret2 = (m_Offset + pos + size <= allSize2);
  ASSERT ( ret2, (m_Offset, pos, size, allSize2) );

  return (ret1 && ret2);
}

void FileReader::SetOffsetAndSize(uint64_t offset, uint64_t size)
{
  ASSERT ( AssertPosAndSize(offset, size), () );
  m_Offset = offset;
  m_Size = size;
}
