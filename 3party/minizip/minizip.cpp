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
  // Locate the entry with an exact, case-sensitive match. We iterate ourselves
  // instead of calling unzLocateFile(): minizip-ng's compat unzLocateFile treats
  // the stored entry name as a wildcard pattern, so it could position on a
  // different entry.
  if (auto const code = GoToFirstFile(file); code != Code::Ok)
    return code;
  do
  {
    FileInfo info;
    if (auto const code = GetCurrentFileInfo(file, info); code != Code::Ok)
      return code;
    if (info.m_filename == filename)
      return Code::Ok;
  }
  while (GoToNextFile(file) == Code::Ok);
  return Code::EndOfListOfFile;
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
  auto const code = static_cast<Code>(unzGetCurrentFileInfo64(file, &info.m_info, nullptr, 0, nullptr, 0, nullptr, 0));
  if (code != Code::Ok)
    return code;

  info.m_filename.resize(info.m_info.size_filename);
  if (info.m_filename.empty())
    return Code::Ok;

  return static_cast<Code>(
      unzGetCurrentFileInfo64(file, nullptr, info.m_filename.data(), info.m_filename.size(), nullptr, 0, nullptr, 0));
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

Code OpenNewFileInZip(File file, std::string const & filename, FileInfo const & fileInfo, std::string const & comment,
                      int method, int level)
{
  auto result =
      zipOpenNewFileInZip(file, filename.c_str(), &fileInfo, nullptr, 0, nullptr, 0, comment.c_str(), method, level);
  return result == 0 ? Code::Ok : Code::InternalError;
}

Code WriteInFileInZip(File file, Buffer const & buf, size_t count)
{
  auto result = zipWriteInFileInZip(file, buf.data(), static_cast<unsigned int>(count));
  return result == 0 ? Code::Ok : Code::InternalError;
}
}  // namespace zip
