#pragma once

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"

#include "base/exception.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

class ZipFileReader : public FileReader
{
public:
  struct Delegate
  {
    virtual ~Delegate() = default;

    // When |size| is zero, end of file is reached.
    virtual void OnBlockUnzipped(size_t size, char const * data) = 0;

    virtual void OnStarted() {}
    virtual void OnCompleted() {}
  };

  using ProgressFn = std::function<void(uint64_t, uint64_t)>;
  /// Contains file name inside zip and it's uncompressed size
  using FileList = std::vector<std::pair<std::string, uint32_t>>;

  DECLARE_EXCEPTION(OpenZipException, OpenException);
  DECLARE_EXCEPTION(LocateZipException, OpenException);
  DECLARE_EXCEPTION(InvalidZipException, OpenException);

  ZipFileReader(std::string const & container, std::string const & file,
                uint32_t logPageSize = FileReader::kDefaultLogPageSize,
                uint32_t logPageCount = FileReader::kDefaultLogPageCount);

  /// @note Size() returns compressed file size inside zip
  uint64_t UncompressedSize() const { return m_uncompressedFileSize; }

  /// @warning Can also throw Writer::OpenException and Writer::WriteException
  static void UnzipFile(std::string const & zipContainer, std::string const & fileInZip, Delegate & delegate);
  static void UnzipFile(std::string const & zipContainer, std::string const & fileInZip, std::string const & outPath);

  static void FilesList(std::string const & zipContainer, FileList & filesList);

  /// Quick version without exceptions
  static bool IsZip(std::string const & zipContainer);

private:
  uint64_t m_uncompressedFileSize;
};
