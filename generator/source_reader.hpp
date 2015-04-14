#pragma once

#include "std/string.hpp"
#include "std/cstdio.hpp"

namespace
{
  class SourceReader
  {
    string const &m_filename;
    FILE * m_file;

  public:
    SourceReader(string const & filename) : m_filename(filename)
    {
      if (m_filename.empty())
      {
        LOG(LINFO, ("Read OSM data from stdin..."));
        m_file = freopen(NULL, "rb", stdin);
      }
      else
      {
        LOG(LINFO, ("Read OSM data from", filename));
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

