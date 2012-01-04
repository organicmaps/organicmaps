#pragma once

#include "mmap_reader.hpp"

#include "../base/exception.hpp"

class ZipFileReader : public MmapReader
{
  typedef MmapReader base_type;

public:
  DECLARE_EXCEPTION(OpenZipException, OpenException);
  DECLARE_EXCEPTION(LocateZipException, OpenException);
  DECLARE_EXCEPTION(InvalidZipException, OpenException);

  ZipFileReader(string const & container, string const & file);

  static vector<string> FilesList(string const & zipContainer);
  /// Quick version without exceptions
  static bool IsZip(string const & zipContainer);
};
