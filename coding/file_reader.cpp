#include "file_reader.hpp"
#include "reader_cache.hpp"
#include "internal/file_data.hpp"
#include "../../base/string_utils.hpp"

#ifndef LOG_FILE_READER_STATS
#define LOG_FILE_READER_STATS 0
#endif // LOG_FILE_READER_STATS

#if LOG_FILE_READER_STATS && !defined(LOG_FILE_READER_EVERY_N_READS_MASK)
#define LOG_FILE_READER_EVERY_N_READS_MASK 0xFFFFFFFF
#endif

namespace
{
  class FileDataWithCachedSize : public FileData
  {
  public:
    explicit FileDataWithCachedSize(string const & fileName)
      : FileData(fileName, FileData::OP_READ), m_Size(static_cast<FileData *>(this)->Size()) {}

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

  string GetName() const { return m_FileData.GetName(); }

private:
  FileDataWithCachedSize m_FileData;
  ReaderCache<FileDataWithCachedSize, LOG_FILE_READER_STATS> m_ReaderCache;

#if LOG_FILE_READER_STATS
  uint32_t m_ReadCallCount;
#endif
};

FileReader::FileReader(string const & fileName, uint32_t logPageSize, uint32_t logPageCount)
  : m_pFileData(new FileReaderData(fileName, logPageSize, logPageCount)),
  m_Offset(0), m_Size(m_pFileData->Size())
{
}

FileReader::FileReader(shared_ptr<FileReaderData> const & pFileData, uint64_t offset, uint64_t size)
  : m_pFileData(pFileData), m_Offset(offset), m_Size(size)
{
}

uint64_t FileReader::Size() const
{
  return m_Size;
}

void FileReader::Read(uint64_t pos, void * p, size_t size) const
{
  ASSERT_LESS_OR_EQUAL(pos + size, Size(), (pos, size));
  m_pFileData->Read(m_Offset + pos, p, size);
}

FileReader FileReader::SubReader(uint64_t pos, uint64_t size) const
{
  ASSERT_LESS_OR_EQUAL(pos + size, Size(), (pos, size));
  return FileReader(m_pFileData, m_Offset + pos, size);
}

FileReader * FileReader::CreateSubReader(uint64_t pos, uint64_t size) const
{
  ASSERT_LESS_OR_EQUAL(pos + size, Size(), (pos, size));
  return new FileReader(m_pFileData, m_Offset + pos, size);
}

bool FileReader::IsEqual(string const & fName) const
{
#if defined(OMIM_OS_WINDOWS)
  return utils::equal_no_case(fName, m_pFileData->GetName());
#else
  return (fName == m_pFileData->GetName());
#endif
}

string FileReader::GetName() const
{
  return m_pFileData->GetName();
}
