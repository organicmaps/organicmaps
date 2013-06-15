#pragma once

#include "file_reader.hpp"

#include "../base/exception.hpp"

#include "../std/function.hpp"


class ZipFileReader : public FileReader
{
private:
  uint64_t m_uncompressedFileSize;

public:
  typedef function<void(int, int)> ProgressFn;
  /// Contains file name inside zip and it's uncompressed size
  typedef vector<pair<string, uint32_t> > FileListT;

  DECLARE_EXCEPTION(OpenZipException, OpenException);
  DECLARE_EXCEPTION(LocateZipException, OpenException);
  DECLARE_EXCEPTION(InvalidZipException, OpenException);

  ZipFileReader(string const & container, string const & file);

  /// @note Size() returns compressed file size inside zip
  uint64_t UncompressedSize() const { return m_uncompressedFileSize; }

  /// @warning Can also throw Writer::OpenException and Writer::WriteException
  static void UnzipFile(string const & zipContainer, string const & fileInZip,
                        string const & outFilePath, ProgressFn progressFn = ProgressFn());

  static void FilesList(string const & zipContainer, FileListT & filesList);

  /// Quick version without exceptions
  static bool IsZip(string const & zipContainer);
};
