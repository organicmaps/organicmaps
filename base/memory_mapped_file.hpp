#pragma once

#include "../std/target_os.hpp"

#ifdef OMIM_OS_WINDOWS_NATIVE
  #include "../std/windows.hpp"
#else
  #include <sys/mman.h>
  #include <sys/errno.h>
  #include <sys/stat.h>
  #include <sys/fcntl.h>
#endif

class MemoryMappedFile
{
private:

  bool m_isReadOnly;

#ifdef OMIM_OS_WINDOWS_NATIVE
  FILE * m_fp;
#else
  int    m_fd;
  void * m_data;
  size_t m_size;
#endif

public:
  MemoryMappedFile(char const * fileName, bool isReadOnly);
  ~MemoryMappedFile();
  void * data();
  size_t size() const;
};
