#pragma once

#include "../std/target_os.hpp"
//#ifdef OMIM_OS_WINDOWS
  #include "file_reader.hpp"
  typedef FileReader BaseZipFileReaderType;
//#else
//  #include "mmap_reader.hpp"
//  typedef MmapReader BaseZipFileReaderType;
//#endif

#include "../base/exception.hpp"

class ZipFileReader : public BaseZipFileReaderType
{
private:
  uint64_t m_uncompressedFileSize;

public:
  DECLARE_EXCEPTION(OpenZipException, OpenException);
  DECLARE_EXCEPTION(LocateZipException, OpenException);
  DECLARE_EXCEPTION(InvalidZipException, OpenException);

  ZipFileReader(string const & container, string const & file);

  /// @note Size() returns compressed file size inside zip
  uint64_t UncompressedSize() const { return m_uncompressedFileSize; }

  /// @warning Can also throw Writer::OpenException and Writer::WriteException
  static void UnzipFile(string const & zipContainer, string const & fileInZip,
                        string const & outFilePath);

  static vector<string> FilesList(string const & zipContainer);
  /// Quick version without exceptions
  static bool IsZip(string const & zipContainer);
};
