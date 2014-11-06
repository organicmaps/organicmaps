#include "platform.hpp"
#include "constants.hpp"

#include "../coding/file_reader.hpp"

#include "../base/regexp.hpp"

#include "../std/target_os.hpp"
#include "../std/algorithm.hpp"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>


ModelReader * Platform::GetReader(string const & file, string const & searchScope) const
{
  return new FileReader(ReadPathForFile(file, searchScope),
                        READER_CHUNK_LOG_SIZE, READER_CHUNK_LOG_COUNT);
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

void Platform::GetFilesByRegExp(string const & directory, string const & regexp, FilesList & outFiles)
{
  regexp::RegExpT exp;
  regexp::Create(regexp, exp);

  QDir dir(QString::fromUtf8(directory.c_str()));
  int const count = dir.count();

  for (int i = 0; i < count; ++i)
  {
    string const name = dir[i].toUtf8().data();
    if (regexp::IsExist(name, exp))
      outFiles.push_back(name);
  }
}

int Platform::PreCachingDepth() const
{
  return 3;
}

int Platform::VideoMemoryLimit() const
{
  return 20 * 1024 * 1024;
}


extern Platform & GetPlatform()
{
  // We need this derive class because Platform::Platform for desktop
  // has special initialization in every platform.
  class PlatformQt : public Platform
  {
  public:
    PlatformQt()
    {
      m_flags.set();
    }
  };

  static PlatformQt platform;
  return platform;
}
