#include "platform.hpp"
#include "constants.hpp"

#include "../coding/file_reader.hpp"

#include "../std/target_os.hpp"
#include "../std/algorithm.hpp"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>


////////////////////////////////////////////////////////////////////////////////////////
ModelReader * Platform::GetReader(string const & file) const
{
  return new FileReader(ReadPathForFile(file),
                        READER_CHUNK_LOG_SIZE, READER_CHUNK_LOG_COUNT);
}

bool Platform::GetFileSizeByFullPath(string const & filePath, uint64_t & size)
{
  QFileInfo f(filePath.c_str());
  size = static_cast<uint64_t>(f.size());
  return size != 0;
}

bool Platform::GetFileSizeByName(string const & fileName, uint64_t & size) const
{
  try
  {
    return GetFileSizeByFullPath(ReadPathForFile(fileName), size);
  }
  catch (RootException const &)
  {
    return false;
  }
}

void Platform::GetFilesInDir(string const & directory, string const & mask, FilesList & outFiles)
{
  QDir dir(directory.c_str(), mask.c_str(), QDir::Unsorted,
           QDir::Files | QDir::Readable | QDir::Dirs | QDir::NoDotAndDotDot);
  int const count = dir.count();
  for (int i = 0; i < count; ++i)
    outFiles.push_back(dir[i].toUtf8().data());
}

string Platform::DeviceName() const
{
  return OMIM_OS_NAME;
}

int Platform::PreCachingDepth() const
{
  return 3;
}

int Platform::ScaleEtalonSize() const
{
  return 512 + 256;
}

int Platform::VideoMemoryLimit() const
{
  return 20 * 1024 * 1024;
}

///////////////////////////////////////////////////////////////////////////////
extern "C" Platform & GetPlatform()
{
  static Platform platform;
  return platform;
}
