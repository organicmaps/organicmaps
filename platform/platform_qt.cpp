#include "platform.hpp"

#include "../base/logging.hpp"

#include "../coding/file_reader.hpp"

#include "../std/target_os.hpp"
#include "../std/algorithm.hpp"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryFile>

////////////////////////////////////////////////////////////////////////////////////////
ModelReader * Platform::GetReader(string const & file) const
{
  return new FileReader(ReadPathForFile(file), 10, 12);
}

bool Platform::GetFileSize(string const & file, uint64_t & size) const
{
  QFileInfo f(file.c_str());
  size = static_cast<uint64_t>(f.size());
  return size != 0;
}

void Platform::GetFilesInDir(string const & directory, string const & mask, FilesList & outFiles) const
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

double Platform::VisualScale() const
{
  return 1.0;
}

string Platform::SkinName() const
{
  return "basic.skn";
}

void Platform::GetFontNames(FilesList & res) const
{
  GetFilesInDir(ResourcesDir(), "*.ttf", res);
  GetFilesInDir(WritableDir(), "*.ttf", res);
  sort(res.begin(), res.end());
  res.erase(unique(res.begin(), res.end()), res.end());
  CHECK(!res.empty(), ("Can't find any valid font in", ResourcesDir(), WritableDir()));
}

int Platform::MaxTilesCount() const
{
  return 120;
}

int Platform::TileSize() const
{
  return 256;
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
