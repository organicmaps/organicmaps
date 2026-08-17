#include "platform/constants.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "coding/file_reader.hpp"

#include "base/logging.hpp"

#include <future>
#include <memory>
#include <regex>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QLocale>

std::unique_ptr<ModelReader> Platform::GetReader(std::string const & file, std::string searchScope) const
{
  return std::make_unique<FileReader>(ReadPathForFile(file, std::move(searchScope)), READER_CHUNK_LOG_SIZE,
                                      READER_CHUNK_LOG_COUNT);
}

bool Platform::GetFileSizeByName(std::string const & fileName, uint64_t & size) const
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

void Platform::GetFilesByRegExp(std::string const & directory, std::string const & regexp, FilesList & outFiles)
{
  std::regex exp(regexp);

  QDir dir(QString::fromUtf8(directory.c_str()));
  int const count = dir.count();

  for (int i = 0; i < count; ++i)
  {
    std::string name = dir[i].toStdString();
    if (std::regex_search(name.begin(), name.end(), exp))
      outFiles.push_back(std::move(name));
  }
}

// static
Platform::EError Platform::MkDir(std::string const & dirName)
{
  // Try to create the directory first: the mkdir syscall is atomic, while a preceding
  // QDir::exists() check would be a TOCTOU race when several processes/threads create the
  // same directory concurrently (e.g. parallel test binaries creating ~/.config/OMaps).
  if (QDir().mkdir(dirName.c_str()))
    return Platform::ERR_OK;
  // mkdir failed: most commonly because the directory already exists. QDir::mkdir() doesn't
  // expose errno, so re-check to map this benign case to ERR_FILE_ALREADY_EXISTS (which
  // MkDirRecursively/MkDirChecked tolerate) instead of a misleading ERR_UNKNOWN.
  if (QDir().exists(dirName.c_str()))
    return Platform::ERR_FILE_ALREADY_EXISTS;
  LOG(LWARNING, ("Can't create directory:", dirName));
  return Platform::ERR_UNKNOWN;
}

void Platform::SetupMeasurementSystem() const
{
  auto units = measurement_utils::Units::Metric;
  if (settings::Get(settings::kMeasurementUnits, units))
    return;
  bool const isMetric = QLocale::system().measurementSystem() == QLocale::MetricSystem;
  units = isMetric ? measurement_utils::Units::Metric : measurement_utils::Units::Imperial;
  settings::Set(settings::kMeasurementUnits, units);
}

extern Platform & GetPlatform()
{
  static Platform platform;
  return platform;
}
