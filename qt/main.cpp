#include "qt/html_processor.hpp"
#include "qt/info_dialog.hpp"
#include "qt/mainwindow.hpp"
#include "qt/screenshoter.hpp"

#include "qt/qt_common/helpers.hpp"

#include "map/framework.hpp"

#include "platform/platform.hpp"
#ifdef OMIM_OS_LINUX
#include "platform/platform_linux_migration.hpp"
#endif
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"
#include "platform/style_utils.hpp"

#include "coding/reader.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

#include "build_style/build_style.h"

#include <QObject>
#include <QtGlobal>
#include <QtGui/QStyleHints>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include <gflags/gflags.h>

#ifdef OMIM_OS_WINDOWS
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <sys/stat.h>
#include <windows.h>
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <utility>
#include <vector>
#endif

DEFINE_string(data_path, "", "Path to data directory.");
DEFINE_string(log_abort_level, base::ToString(base::GetDefaultLogAbortLevel()),
              "Log messages severity that causes termination.");
DEFINE_string(resources_path, "", "Path to resources directory.");
DEFINE_string(kml_path, "",
              "Activates screenshot mode. Path to a kml file or a directory with kml files to take screenshots.");
DEFINE_string(points, "",
              "Activates screenshot mode. Points on the map and zoom level "
              "[1..18] in format \"lat,lon,zoom[;lat,lon,zoom]\" or path to a file with points in "
              "the same format. Each point and zoom define a place on the map to take screenshot.");
DEFINE_string(rects, "",
              "Activates screenshot mode. Rects on the map in format"
              "\"lat_leftBottom,lon_leftBottom,lat_rightTop,lon_rightTop"
              "[;lat_leftBottom,lon_leftBottom,lat_rightTop,lon_rightTop]\" or path to a file with "
              "rects in the same format. Each rect defines a place on the map to take screenshot.");
DEFINE_string(dst_path, "", "Path to a directory to save screenshots.");
DEFINE_string(lang, "", "Preferred language override.");
DEFINE_int32(width, 0, "Screenshot width.");
DEFINE_int32(height, 0, "Screenshot height.");
DEFINE_double(
    dpi_scale, 0.0,
    "Screenshot dpi scale (mdpi = 1.0, hdpi = 1.5, xhdpiScale = 2.0, 6plus = 2.4, xxhdpi = 3.0, xxxhdpi = 3.5).");

namespace
{
bool ValidateLogAbortLevel(char const * flagname, std::string const & value)
{
  if (auto level = base::FromString(value); !level)
  {
    std::cerr << "Invalid value for --" << flagname << ": " << value << ", must be one of: ";
    auto const & names = base::GetLogLevelNames();
    for (size_t i = 0; i < names.size(); ++i)
    {
      if (i != 0)
        std::cerr << ", ";
      std::cerr << names[i];
    }
    std::cerr << '\n';
    return false;
  }
  return true;
}

bool const g_logAbortLevelDummy = gflags::RegisterFlagValidator(&FLAGS_log_abort_level, &ValidateLogAbortLevel);

#if defined(OMIM_OS_WINDOWS)
void PruneWindowsLogs(std::filesystem::path const & logDir, std::filesystem::path const & currentLog)
{
  using LogFile = std::pair<std::filesystem::file_time_type, std::filesystem::path>;
  std::vector<LogFile> logs;
  std::error_code error;
  for (std::filesystem::directory_iterator it(logDir, error), end; !error && it != end; it.increment(error))
  {
    if (it->path() == currentLog)
      continue;
    auto const name = it->path().filename().string();
    auto const status = it->symlink_status(error);
    if (error)
    {
      error.clear();
      continue;
    }
    if (!std::filesystem::is_regular_file(status) || !name.starts_with("organicmaps-") || !name.ends_with(".log"))
      continue;
    auto const modified = it->last_write_time(error);
    if (error)
    {
      error.clear();
      continue;
    }
    logs.emplace_back(modified, it->path());
  }
  std::sort(logs.begin(), logs.end(), std::greater<>());
  constexpr size_t kPreviousLogsToKeep = 9;
  for (size_t i = kPreviousLogsToKeep; i < logs.size(); ++i)
  {
    error.clear();
    std::filesystem::remove(logs[i].second, error);  // Open logs are retried on a later launch.
  }
}

class InitializeFinalize
{
public:
  InitializeFinalize()
  {
    auto const logDir = GetPlatform().WritableDir() + "logs\\";
    if (!Platform::MkDirRecursively(logDir))
      return;

    SYSTEMTIME now;
    ::GetSystemTime(&now);
    char timestamp[40];
    std::snprintf(timestamp, sizeof(timestamp), "organicmaps-%04u%02u%02u-%02u%02u%02u-%03u-",
                  static_cast<unsigned>(now.wYear), static_cast<unsigned>(now.wMonth), static_cast<unsigned>(now.wDay),
                  static_cast<unsigned>(now.wHour), static_cast<unsigned>(now.wMinute),
                  static_cast<unsigned>(now.wSecond), static_cast<unsigned>(now.wMilliseconds));

    for (unsigned int suffix = 0; suffix < 1000; ++suffix)
    {
      auto const logPath = logDir + timestamp + std::to_string(suffix) + ".log";
      int fd = -1;
      auto const result =
          ::_sopen_s(&fd, logPath.c_str(), _O_WRONLY | _O_CREAT | _O_EXCL | _O_TEXT, _SH_DENYNO, _S_IREAD | _S_IWRITE);
      if (result == EEXIST)
        continue;
      if (result != 0)
      {
        LOG(LWARNING, ("Could not create log file", logPath, result));
        return;
      }

      ::_close(fd);
      if (::freopen(logPath.c_str(), "a", stderr) == nullptr)
        return;
      PruneWindowsLogs(logDir, logPath);
      LOG(LINFO, ("Logging to", logPath));
      return;
    }
  }
  ~InitializeFinalize() = default;
};
#else
class InitializeFinalize
{
public:
  InitializeFinalize() = default;
  ~InitializeFinalize() = default;
};
#endif
}  // namespace

int main(int argc, char * argv[])
{
  // Our double parsing code (base/string_utils.hpp) needs dots as a floating point delimiters, not commas.
  // TODO: Refactor our doubles parsing code to use locale-independent delimiters.
  // For example, https://github.com/google/double-conversion can be used.
  // See http://dbaron.org/log/20121222-locale for more details.
  std::setlocale(LC_NUMERIC, "C");

#ifdef OMIM_OS_LINUX
  platform::EnableDesktopDataMigration();
#endif
  Platform & platform = GetPlatform();

  LOG(LINFO, ("Organic Maps", platform.Version(), "built with QT:", QT_VERSION_STR, "runtime QT:", qVersion(),
              "detected CPU cores:", platform.CpuCores()));

  gflags::SetUsageMessage("Desktop application.");
  gflags::SetVersionString(platform.Version());
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  if (!FLAGS_lang.empty())
    languages::SetPreferredLanguageOverride(FLAGS_lang);

  if (!FLAGS_resources_path.empty())
    platform.SetResourceDir(FLAGS_resources_path);
  if (!FLAGS_data_path.empty())
    platform.SetWritableDirForTests(FLAGS_data_path);

  if (auto const logLevel = base::FromString(FLAGS_log_abort_level); logLevel)
    base::g_LogAbortLevel = *logLevel;
  else
    LOG(LCRITICAL, ("Invalid log level:", FLAGS_log_abort_level));

  Q_INIT_RESOURCE(resources_common);

  InitializeFinalize mainGuard;
  UNUSED_VALUE(mainGuard);

  QApplication app(argc, argv);
  app.setDesktopFileName("app.organicmaps.desktop");

#ifdef BUILD_DESIGNER
  QApplication::setApplicationName("Organic Maps Designer");
#else
  QApplication::setApplicationName("Organic Maps");
#endif

#ifdef DEBUG
  static bool constexpr developerMode = true;
#else
  static bool constexpr developerMode = false;
#endif
  bool outvalue;
  if (!settings::Get(settings::kDeveloperMode, outvalue))
    settings::Set(settings::kDeveloperMode, developerMode);

  // Display EULA if needed.
  char const * settingsEULA = "EulaAccepted";
  bool eulaAccepted = false;
  if (!settings::Get(settingsEULA, eulaAccepted) || !eulaAccepted)
  {
    std::string buffer;
    {
      ReaderPtr<Reader> reader = platform.GetReader("copyright.html");
      reader.ReadAsString(buffer);
    }
    RemovePTagsWithNonMatchedLanguages(languages::GetCurrentTwine(), buffer);
    qt::InfoDialog eulaDialog(QCoreApplication::applicationName(), buffer.c_str(), nullptr, {"Accept", "Decline"});
    eulaAccepted = (eulaDialog.exec() == 1);
    settings::Set(settingsEULA, eulaAccepted);
  }

  int returnCode = -1;
  if (eulaAccepted)  // User has accepted EULA
  {
    std::unique_ptr<qt::ScreenshotParams> screenshotParams;

    if (!FLAGS_kml_path.empty() || !FLAGS_points.empty() || !FLAGS_rects.empty())
    {
      screenshotParams = std::make_unique<qt::ScreenshotParams>();
      if (!FLAGS_kml_path.empty())
      {
        screenshotParams->m_kmlPath = FLAGS_kml_path;
        screenshotParams->m_mode = qt::ScreenshotParams::Mode::KmlFiles;
      }
      else if (!FLAGS_points.empty())
      {
        screenshotParams->m_points = FLAGS_points;
        screenshotParams->m_mode = qt::ScreenshotParams::Mode::Points;
      }
      else if (!FLAGS_rects.empty())
      {
        screenshotParams->m_rects = FLAGS_rects;
        screenshotParams->m_mode = qt::ScreenshotParams::Mode::Rects;
      }
      if (!FLAGS_dst_path.empty())
        screenshotParams->m_dstPath = FLAGS_dst_path;
      if (FLAGS_width > 0)
        screenshotParams->m_width = FLAGS_width;
      if (FLAGS_height > 0)
        screenshotParams->m_height = FLAGS_height;
      if (FLAGS_dpi_scale >= df::VisualParams::kMdpiScale && FLAGS_dpi_scale <= df::VisualParams::kXxxhdpiScale)
        screenshotParams->m_dpiScale = FLAGS_dpi_scale;
    }

    qt::common::SetDefaultSurfaceFormat(QApplication::platformName());

    FrameworkParams frameworkParams;

#ifdef BUILD_DESIGNER
    QString mapcssFilePath;
    if (argc >= 2 && platform.IsFileExistsByFullPath(argv[1]))
      mapcssFilePath = argv[1];
    if (0 == mapcssFilePath.length())
      mapcssFilePath = QFileDialog::getOpenFileName(nullptr, "Open style.mapcss file", "~/", "MapCSS Files (*.mapcss)");
    if (mapcssFilePath.isEmpty())
      return returnCode;

    try
    {
      build_style::BuildIfNecessaryAndApply(mapcssFilePath);
    }
    catch (std::exception const & e)
    {
      QMessageBox msgBox;
      msgBox.setWindowTitle("Error");
      msgBox.setText(e.what());
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.setDefaultButton(QMessageBox::Ok);
      msgBox.exec();
      return returnCode;
    }

#endif  // BUILD_DESIGNER

    Framework framework(frameworkParams);
    framework.SetupMeasurementSystem();

    auto const syncNightMode = [&framework]()
    {
      if (style_utils::GetNightModeSetting() == style_utils::NightMode::System)
        qt::common::ApplySystemNightMode(framework);
    };
    syncNightMode();
    qt::MainWindow w(framework, std::move(screenshotParams), QApplication::primaryScreen()->geometry()
#ifdef BUILD_DESIGNER
                                                                 ,
                     mapcssFilePath
#endif  // BUILD_DESIGNER
    );
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (auto * styleHints = QGuiApplication::styleHints(); styleHints != nullptr)
    {
      QObject::connect(styleHints, &QStyleHints::colorSchemeChanged, &w,
                       [syncNightMode](Qt::ColorScheme) mutable { syncNightMode(); });
    }
#endif
    w.show();
    returnCode = QApplication::exec();
  }

#ifdef BUILD_DESIGNER
  if (build_style::NeedRecalculate && !mapcssFilePath.isEmpty())
  {
    try
    {
      build_style::RunRecalculationGeometryScript(mapcssFilePath);
    }
    catch (std::exception & e)
    {
      QMessageBox msgBox;
      msgBox.setWindowTitle("Error");
      msgBox.setText(e.what());
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.setDefaultButton(QMessageBox::Ok);
      msgBox.exec();
    }
  }
#endif  // BUILD_DESIGNER

  LOG_SHORT(LINFO, ("Organic Maps finished with code", returnCode));
  return returnCode;
}
