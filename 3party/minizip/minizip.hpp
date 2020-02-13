#pragma once

#include "3party/minizip/src/unzip.h"
#include "3party/minizip/src/zip.h"

#include <array>
#include <cstdint>
#include <limits>
#include <string>

namespace unzip
{
unsigned int static constexpr kFileBufferSize = 64 * 1024;
static_assert(std::numeric_limits<int>::max() > kFileBufferSize, "");

using File = unzFile;
using FilePos = ZPOS64_T;
using Buffer = std::array<char, kFileBufferSize>;

struct FileInfo
{
  std::string m_filename;
  unz_file_info64 m_info;
};

enum class Code : int8_t
{
  Ok = UNZ_OK,
  EndOfListOfFile = UNZ_END_OF_LIST_OF_FILE,
  ErrNo = UNZ_ERRNO,
  Eof = UNZ_EOF,
  ParamError = UNZ_PARAMERROR,
  BadZipFile = UNZ_BADZIPFILE,
  InternalError = UNZ_INTERNALERROR,
  CrcError = UNZ_CRCERROR,
};

File Open(std::string const & filename);

Code Close(File file);

Code GoToFirstFile(File file);

Code GoToNextFile(File file);

Code GoToFile(File file, std::string const & filename);

Code OpenCurrentFile(File file);

FilePos GetCurrentFileFilePos(File file);

Code CloseCurrentFile(File file);

Code GetCurrentFileInfo(File file, FileInfo & info);

int ReadCurrentFile(File file, Buffer & result);
}  // namespace unzip

namespace zip
{
unsigned int constexpr kFileBufferSize = 64 * 1024;

using File = zipFile;
using FileInfo = zip_fileinfo;
using DateTime = tm_zip;
using Buffer = std::array<char, kFileBufferSize>;

enum class Code : int8_t
{
  Ok = ZIP_OK,
  Eof = ZIP_EOF,
  ErrNo = ZIP_ERRNO,
  ParamError = ZIP_PARAMERROR,
  BadZipFile = ZIP_BADZIPFILE,
  InternalError = ZIP_INTERNALERROR,
};

File Create(std::string const & filename);

Code Close(File file);

Code OpenNewFileInZip(File file, std::string const & filename, FileInfo const & fileInfo,
                      std::string const & comment, int method, int level);

Code WriteInFileInZip(File file, Buffer const & buf, size_t count);
}  // namespace zip
