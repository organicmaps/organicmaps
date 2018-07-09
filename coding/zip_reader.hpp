#pragma once

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"

#include "base/exception.hpp"

#include "std/function.hpp"
#include "std/utility.hpp"


class ZipFileReader : public FileReader
{
private:
  uint64_t m_uncompressedFileSize;

public:
  struct Delegate
  {
    virtual ~Delegate() = default;

    // When |size| is zero, end of file is reached.
    virtual void OnBlockUnzipped(size_t size, char const * data) = 0;

    virtual void OnStarted() {}
    virtual void OnCompleted() {}
  };

  typedef function<void(uint64_t, uint64_t)> ProgressFn;
  /// Contains file name inside zip and it's uncompressed size
  typedef vector<pair<string, uint32_t> > FileListT;

  DECLARE_EXCEPTION(OpenZipException, OpenException);
  DECLARE_EXCEPTION(LocateZipException, OpenException);
  DECLARE_EXCEPTION(InvalidZipException, OpenException);

  ZipFileReader(string const & container, string const & file,
                uint32_t logPageSize = FileReader::kDefaultLogPageSize,
                uint32_t logPageCount = FileReader::kDefaultLogPageCount);

  /// @note Size() returns compressed file size inside zip
  uint64_t UncompressedSize() const { return m_uncompressedFileSize; }

  /// @warning Can also throw Writer::OpenException and Writer::WriteException
  static void UnzipFile(string const & zipContainer, string const & fileInZip, Delegate & delegate);
  static void UnzipFile(string const & zipContainer, string const & fileInZip,
                        string const & outPath);

  static void FilesList(string const & zipContainer, FileListT & filesList);

  /// Quick version without exceptions
  static bool IsZip(string const & zipContainer);
};
