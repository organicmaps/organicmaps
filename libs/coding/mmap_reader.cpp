#include "coding/mmap_reader.hpp"

#include "base/scope_guard.hpp"

#include "std/target_os.hpp"

#include <cstring>

#ifdef OMIM_OS_WINDOWS
#include "std/windows.hpp"
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

class MmapReader::MmapData
{
public:
  explicit MmapData(std::string const & fileName, Advice advice)
  {
#ifdef OMIM_OS_WINDOWS
    m_hFile = CreateFileA(fileName.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (m_hFile == INVALID_HANDLE_VALUE)
      MYTHROW(Reader::OpenException, ("Can't open file:", fileName, "win last error:", GetLastError()));

    SCOPE_GUARD(fileGuard, [this] { CloseHandle(m_hFile); });

    m_hMapping = CreateFileMappingA(m_hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!m_hMapping)
      MYTHROW(Reader::OpenException,
              ("Can't create file's Windows mapping:", fileName, "win last error:", GetLastError()));

    SCOPE_GUARD(mappingGuard, [this] { CloseHandle(m_hMapping); });

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(m_hFile, &fileSize))
      MYTHROW(Reader::OpenException, ("Can't get file size:", fileName, "win last error:", GetLastError()));

    m_size = fileSize.QuadPart;
    m_memory = static_cast<uint8_t *>(MapViewOfFile(m_hMapping, FILE_MAP_READ, 0, 0, 0));
    if (!m_memory)
      MYTHROW(Reader::OpenException,
              ("Can't create file's Windows mapping:", fileName, "win last error:", GetLastError()));

    mappingGuard.release();
    fileGuard.release();
#else
    m_fd = open(fileName.c_str(), O_RDONLY | O_NONBLOCK);
    if (m_fd == -1)
      MYTHROW(OpenException, ("open failed for file", fileName));

    struct stat s;
    if (-1 == fstat(m_fd, &s))
      MYTHROW(OpenException, ("fstat failed for file", fileName));
    m_size = s.st_size;

    m_memory = static_cast<uint8_t *>(mmap(0, static_cast<size_t>(m_size), PROT_READ, MAP_PRIVATE, m_fd, 0));
    if (m_memory == MAP_FAILED)
    {
      close(m_fd);
      MYTHROW(OpenException, ("mmap failed for file", fileName));
    }

    int adv = MADV_NORMAL;
    switch (advice)
    {
    case Advice::Random: adv = MADV_RANDOM; break;
    case Advice::Sequential: adv = MADV_SEQUENTIAL; break;
    case Advice::Normal: adv = MADV_NORMAL; break;
    }

    if (madvise(m_memory, static_cast<size_t>(s.st_size), adv) != 0)
      LOG(LWARNING, ("madvise error:", strerror(errno)));
#endif
  }

  ~MmapData()
  {
#ifdef OMIM_OS_WINDOWS
    UnmapViewOfFile(m_memory);

    CloseHandle(m_hMapping);
    CloseHandle(m_hFile);
#else
    munmap(m_memory, static_cast<size_t>(m_size));
    close(m_fd);
#endif
  }

  uint8_t * m_memory = nullptr;
  uint64_t m_size = 0;

private:
#ifdef OMIM_OS_WINDOWS
  HANDLE m_hFile;
  HANDLE m_hMapping;
#else
  int m_fd = 0;
#endif
};

MmapReader::MmapReader(std::string const & fileName, Advice advice)
  : base_type(fileName)
  , m_data(std::make_shared<MmapData>(fileName, advice))
  , m_offset(0)
  , m_size(m_data->m_size)
{}

MmapReader::MmapReader(MmapReader const & reader, uint64_t offset, uint64_t size)
  : base_type(reader.GetName())
  , m_data(reader.m_data)
  , m_offset(offset)
  , m_size(size)
{}

uint64_t MmapReader::Size() const
{
  return m_size;
}

void MmapReader::Read(uint64_t pos, void * p, size_t size) const
{
  ASSERT_LESS_OR_EQUAL(pos + size, Size(), (pos, size));
  memcpy(p, m_data->m_memory + m_offset + pos, size);
}

std::unique_ptr<Reader> MmapReader::CreateSubReader(uint64_t pos, uint64_t size) const
{
  ASSERT_LESS_OR_EQUAL(pos + size, Size(), (pos, size));
  // Can't use make_unique with private constructor.
  return std::unique_ptr<Reader>(new MmapReader(*this, m_offset + pos, size));
}

uint8_t * MmapReader::Data() const
{
  return m_data->m_memory;
}

void MmapReader::SetOffsetAndSize(uint64_t offset, uint64_t size)
{
  ASSERT_LESS_OR_EQUAL(offset + size, Size(), (offset, size));
  m_offset = offset;
  m_size = size;
}
