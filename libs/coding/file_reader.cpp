#include "coding/file_reader.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/reader_cache.hpp"

#include "base/logging.hpp"

#ifndef LOG_FILE_READER_STATS
#define LOG_FILE_READER_STATS 0
#endif  // LOG_FILE_READER_STATS

#if LOG_FILE_READER_STATS && !defined(LOG_FILE_READER_EVERY_N_READS_MASK)
#define LOG_FILE_READER_EVERY_N_READS_MASK 0xFFFFFFFF
#endif

// static
uint32_t const FileReader::kDefaultLogPageSize = 10;  // page size is 2^10 = 1024 = 1kb
// static
uint32_t const FileReader::kDefaultLogPageCount = 4;  // page count is 2^4 = 16, i.e. 16 pages are cached

class FileReader::FileReaderData
{
public:
  FileReaderData(std::string const & fileName, uint32_t logPageSize, uint32_t logPageCount)
    : m_fileData(fileName)
    , m_readerCache(logPageSize, logPageCount)
  {
#if LOG_FILE_READER_STATS
    m_readCallCount = 0;
#endif
  }

  ~FileReaderData()
  {
#if LOG_FILE_READER_STATS
    LOG(LINFO, ("FileReader", m_fileData.GetName(), m_readerCache.GetStatsStr()));
#endif
  }

  uint64_t Size() const { return m_fileData.Size(); }

  void Read(uint64_t pos, void * p, size_t size)
  {
#if LOG_FILE_READER_STATS
    if (((++m_readCallCount) & LOG_FILE_READER_EVERY_N_READS_MASK) == 0)
      LOG(LINFO, ("FileReader", m_fileData.GetName(), m_readerCache.GetStatsStr()));
#endif

    return m_readerCache.Read(m_fileData, pos, p, size);
  }

private:
  class FileDataWithCachedSize : public base::FileData
  {
  public:
    explicit FileDataWithCachedSize(std::string const & fileName)
      : base::FileData(fileName, Op::READ)
      , m_Size(FileData::Size())
    {}

    uint64_t Size() const { return m_Size; }

  private:
    uint64_t m_Size;
  };

  FileDataWithCachedSize m_fileData;
  ReaderCache<FileDataWithCachedSize, LOG_FILE_READER_STATS> m_readerCache;

#if LOG_FILE_READER_STATS
  uint32_t m_readCallCount;
#endif
};

FileReader::FileReader(std::string const & fileName) : FileReader(fileName, kDefaultLogPageSize, kDefaultLogPageCount)
{}

FileReader::FileReader(std::string const & fileName, uint32_t logPageSize, uint32_t logPageCount)
  : ModelReader(fileName)
  , m_logPageSize(logPageSize)
  , m_logPageCount(logPageCount)
  , m_fileData(std::make_shared<FileReaderData>(fileName, logPageSize, logPageCount))
  , m_offset(0)
  , m_size(m_fileData->Size())
{}

FileReader::FileReader(FileReader const & reader, uint64_t offset, uint64_t size, uint32_t logPageSize,
                       uint32_t logPageCount)
  : ModelReader(reader.GetName())
  , m_logPageSize(logPageSize)
  , m_logPageCount(logPageCount)
  , m_fileData(reader.m_fileData)
  , m_offset(offset)
  , m_size(size)
{}

void FileReader::Read(uint64_t pos, void * p, size_t size) const
{
  CheckPosAndSize(pos, size);
  m_fileData->Read(m_offset + pos, p, size);
}

FileReader FileReader::SubReader(uint64_t pos, uint64_t size) const
{
  CheckPosAndSize(pos, size);
  return FileReader(*this, m_offset + pos, size, m_logPageSize, m_logPageCount);
}

std::unique_ptr<Reader> FileReader::CreateSubReader(uint64_t pos, uint64_t size) const
{
  CheckPosAndSize(pos, size);
  // Can't use make_unique with private constructor.
  return std::unique_ptr<Reader>(new FileReader(*this, m_offset + pos, size, m_logPageSize, m_logPageCount));
}

void FileReader::CheckPosAndSize(uint64_t pos, uint64_t size) const
{
  uint64_t const allSize1 = Size();
  bool const ret1 = (pos + size <= allSize1);
  if (!ret1)
    MYTHROW(Reader::SizeException, (pos, size, allSize1));

  uint64_t const allSize2 = m_fileData->Size();
  bool const ret2 = (m_offset + pos + size <= allSize2);
  if (!ret2)
    MYTHROW(Reader::SizeException, (pos, size, allSize2));
}

void FileReader::SetOffsetAndSize(uint64_t offset, uint64_t size)
{
  CheckPosAndSize(offset, size);
  m_offset = offset;
  m_size = size;
}
