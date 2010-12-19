#include "../base/SRC_FIRST.hpp"
#include "memory_mapped_file.hpp"

MemoryMappedFile::MemoryMappedFile(char const * fileName, bool isReadOnly)
  : m_isReadOnly(isReadOnly)
{
#ifdef OMIM_OS_WINDOWS_NATIVE
  m_fp = fopen(name, isReadOnly ? "r" : "w");
  fseek(m_fp, SEEK_END);
  m_size = ftell(m_fp);
  fseek(m_fp, SEEK_SET);

  m_data = malloc(m_size);
  fread(m_data, 1, m_size, m_fp);
#else
  struct stat s;
  stat(fileName, &s);
  m_size = s.st_size;
  m_fd = open(fileName, isReadOnly ? O_RDONLY : O_RDWR);
  m_data = mmap(0, m_size, isReadOnly ? PROT_READ : (PROT_READ | PROT_WRITE), MAP_SHARED, m_fd, 0);
#endif
}

MemoryMappedFile::~MemoryMappedFile()
{
#ifdef OMIM_OS_WINDOWS_NATIVE
  if (!m_isReadOnly)
  {
    fwrite(m_data, 1, m_size, m_fp);
  }
  fclose(m_fp);
  free(m_data);
#else
  munmap(m_data, m_size);
  close(m_fd);
#endif
}

void * MemoryMappedFile::data()
{
  return m_data;
}

size_t MemoryMappedFile::size() const
{
  return m_size;
}
