#include "minizip.hpp"

namespace unzip
{
File Open(std::string const & filename)
{
  return unzOpen64(filename.c_str());
}

Code Close(File file)
{
  return static_cast<Code>(unzClose(file));
}

Code GoToFirstFile(File file)
{
  return static_cast<Code>(unzGoToFirstFile(file));
}

Code GoToNextFile(File file)
{
  return static_cast<Code>(unzGoToNextFile(file));
}

Code GoToFile(File file, std::string const & filename)
{
  return static_cast<Code>(unzLocateFile(file, filename.c_str(), 1 /* iCaseSensitivity */));
}

Code OpenCurrentFile(File file)
{
  return static_cast<Code>(unzOpenCurrentFile(file));
}

FilePos GetCurrentFileFilePos(File file)
{
  return unzGetCurrentFileZStreamPos64(file);
}

Code CloseCurrentFile(File file)
{
  return static_cast<Code>(unzCloseCurrentFile(file));
}

Code GetCurrentFileInfo(File file, FileInfo & info)
{
  int constexpr kArraySize = 256;
  char fileName[kArraySize];
  auto const result = unzGetCurrentFileInfo64(file, &info.m_info, fileName,
                                              kArraySize, nullptr, 0, nullptr, 0);
  info.m_filename = fileName;
  return static_cast<Code>(result);
}

int ReadCurrentFile(unzFile file, Buffer & result)
{
  auto const readCount = unzReadCurrentFile(file, result.data(), kFileBufferSize);
  return readCount;
}
}  // namespace unzip

namespace zip
{
File Create(std::string const & filename)
{
  return zipOpen(filename.c_str(), APPEND_STATUS_CREATE);
}

Code Close(File file)
{
  return static_cast<Code>(zipClose(file, nullptr));
}

Code OpenNewFileInZip(File file, std::string const & filename, FileInfo const & fileInfo,
                      std::string const & comment, int method, int level)
{
  auto result = zipOpenNewFileInZip(file, filename.c_str(), &fileInfo, nullptr, 0,
                                    nullptr, 0, comment.c_str(), Z_DEFLATED, Z_DEFAULT_COMPRESSION);
  return result == 0 ? Code::Ok : Code::InternalError;
}

Code WriteInFileInZip(File file, Buffer const & buf, size_t count)
{
  auto result = zipWriteInFileInZip(file, buf.data(), static_cast<unsigned int>(count));
  return result == 0 ? Code::Ok : Code::InternalError;
}
}  // namespace zip
