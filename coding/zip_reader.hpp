#pragma once

#include "../std/target_os.hpp"
#ifdef OMIM_OS_WINDOWS
  #include "file_reader.hpp"
  typedef FileReader BaseZipFileReaderType;
#else
  #include "mmap_reader.hpp"
  typedef MmapReader BaseZipFileReaderType;
#endif

#include "../base/exception.hpp"

class ZipFileReader : public BaseZipFileReaderType
{
public:
  DECLARE_EXCEPTION(OpenZipException, OpenException);
  DECLARE_EXCEPTION(LocateZipException, OpenException);
  DECLARE_EXCEPTION(InvalidZipException, OpenException);

  ZipFileReader(string const & container, string const & file);

  static vector<string> FilesList(string const & zipContainer);
  /// Quick version without exceptions
  static bool IsZip(string const & zipContainer);
};
