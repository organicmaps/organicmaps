#pragma once

#include "../base/assert.hpp"

#include "../std/string.hpp"

/// Should be incremented when binary format changes
uint32_t const MAPS_MAJOR_VERSION_BINARY_FORMAT = 0;

#define DATA_FILE_EXTENSION ".dat"

#define UPDATE_CHECK_FILE "maps.update"
#define UPDATE_BASE_URL "http://melnichek.ath.cx:34568/maps/"
#define UPDATE_FULL_URL UPDATE_BASE_URL UPDATE_CHECK_FILE

namespace mapinfo
{
  inline bool IsDatFile(string const & fileName)
  {
    /// file name ends with data file extension
    string const ext(DATA_FILE_EXTENSION);
    return fileName.rfind(ext) == fileName.size() - ext.size();
  }

  inline string IndexFileForDatFile(string const & fileName)
  {
    ASSERT(IsDatFile(fileName), ());
    static char const * INDEX_FILE_EXTENSION = ".idx";
    return fileName + INDEX_FILE_EXTENSION;
  }
}
