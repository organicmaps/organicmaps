#include "coding/mmap_reader.hpp"

#include "std/target_os.hpp"
#include "std/cstring.hpp"

// @TODO we don't support windows at the moment
#ifndef OMIM_OS_WINDOWS
  #include <unistd.h>
  #include <sys/mman.h>
  #include <sys/stat.h>
  #ifdef OMIM_OS_ANDROID
    #include <fcntl.h>
  #else
    #include <sys/fcntl.h>
  #endif
#endif

class MmapReader::MmapData
{
  int m_fd;

public:
  uint8_t * m_memory;
  uint64_t m_size;

  MmapData(string const & fileName)
  {
    // @TODO add windows support
#ifndef OMIM_OS_WINDOWS
    m_fd = open(fileName.c_str(), O_RDONLY | O_NONBLOCK);
    if (m_fd == -1)
      MYTHROW(OpenException, ("open failed for file", fileName));

    struct stat s;
    if (-1 == fstat(m_fd, &s))
      MYTHROW(OpenException, ("fstat failed for file", fileName));
    m_size = s.st_size;

    m_memory = (uint8_t *)mmap(0, m_size, PROT_READ, MAP_SHARED, m_fd, 0);
    if (m_memory == MAP_FAILED)
    {
      close(m_fd);
      MYTHROW(OpenException, ("mmap failed for file", fileName));
    }
#endif
  }

  ~MmapData()
  {
    // @TODO add windows support
#ifndef OMIM_OS_WINDOWS
    munmap(m_memory, m_size);
    close(m_fd);
#endif
  }
};

MmapReader::MmapReader(string const & fileName)
  : base_type(fileName), m_offset(0)
{
  m_data = shared_ptr<MmapData>(new MmapData(fileName));
  m_size = m_data->m_size;
}

MmapReader::MmapReader(MmapReader const & reader, uint64_t offset, uint64_t size)
  : base_type(reader.GetName()), m_data(reader.m_data),
    m_offset(offset), m_size(size)
{
}

uint64_t MmapReader::Size() const
{
  return m_size;
}

void MmapReader::Read(uint64_t pos, void * p, size_t size) const
{
  ASSERT_LESS_OR_EQUAL(pos + size, Size(), (pos, size));
  memcpy(p, m_data->m_memory + m_offset + pos, size);
}

unique_ptr<Reader> MmapReader::CreateSubReader(uint64_t pos, uint64_t size) const
{
  ASSERT_LESS_OR_EQUAL(pos + size, Size(), (pos, size));
  // Can't use make_unique with private constructor.
  return unique_ptr<Reader>(new MmapReader(*this, m_offset + pos, size));
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
