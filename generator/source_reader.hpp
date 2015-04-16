#pragma once

#include "std/cstdio.hpp"
#include "std/string.hpp"

namespace
{
class SourceReader
{
  // TODO (@yershov): SourceReader ctor is quite bizarre since it reads data from stdin when
  // filename is empty or opens file otherwise. There two behaviors are completely different and
  // should be separated to different methods.
  string const & m_filename;
  FILE * m_file;

public:
  SourceReader(string const & filename) : m_filename(filename)
  {
    if (m_filename.empty())
    {
      LOG(LINFO, ("Reading OSM data from stdin"));
      m_file = freopen(nullptr, "rb", stdin);
    }
    else
    {
      LOG(LINFO, ("Reading OSM data from", filename));
      m_file = fopen(filename.c_str(), "rb");
    }
  }

  ~SourceReader()
  {
    if (!m_filename.empty())
      fclose(m_file);
  }

  inline FILE * Handle() { return m_file; }

  uint64_t Read(char * buffer, uint64_t bufferSize)
  {
    return fread(buffer, sizeof(char), bufferSize, m_file);
  }
};
}
