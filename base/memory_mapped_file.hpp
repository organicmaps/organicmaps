#pragma once
#include "base.hpp"

#include "../std/target_os.hpp"

#include "../std/windows.hpp"
#include "../std/cstdio.hpp"

class MemoryMappedFile
{
  bool m_isReadOnly;

#ifdef OMIM_OS_WINDOWS
  FILE * m_fp;
#else
  int    m_fd;
#endif

  void * m_data;
  size_t m_size;

public:
  MemoryMappedFile(char const * fileName, bool isReadOnly);
  ~MemoryMappedFile();

  void * data();
  size_t size() const;
};
