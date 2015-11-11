
#include "platform/constants.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "coding/file_reader.hpp"

#include "base/logging.hpp"

#include "std/algorithm.hpp"
#include "std/regex.hpp"
#include "std/target_os.hpp"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>

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
  regex exp(regexp);

  QDir dir(QString::fromUtf8(directory.c_str()));
  int const count = dir.count();

  for (int i = 0; i < count; ++i)
  {
    string const name = dir[i].toUtf8().data();
    if (regex_search(name.begin(), name.end(), exp))
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

Platform::EError Platform::MkDir(string const & dirName) const
{
  if (QDir().exists(dirName.c_str()))
    return Platform::ERR_FILE_ALREADY_EXISTS;
  if(!QDir().mkdir(dirName.c_str()))
  {
    LOG(LWARNING, ("Can't create directory: ", dirName));
    return Platform::ERR_UNKNOWN;
  }
  return Platform::ERR_OK;
}

void Platform::SetupMeasurementSystem() const
{
  Settings::Units u;
  if (Settings::Get("Units", u))
    return;
  bool const isMetric = QLocale::system().measurementSystem() == QLocale::MetricSystem;
  u = isMetric ? Settings::Metric : Settings::Foot;
  Settings::Set("Units", u);
}

extern Platform & GetPlatform()
{
  static Platform platform;
  return platform;
}
