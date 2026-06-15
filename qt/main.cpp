#include "qt/html_processor.hpp"
#include "qt/info_dialog.hpp"
#include "qt/mainwindow.hpp"
#include "qt/screenshoter.hpp"

#include "qt/qt_common/helpers.hpp"

#include "map/framework.hpp"

#include "indexer/map_style_reader.hpp"

#include "platform/platform.hpp"
#ifdef OMIM_OS_LINUX
#include "platform/platform_linux_migration.hpp"
#endif
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"
#include "platform/style_utils.hpp"

#include "coding/reader.hpp"

#include "base/logging.hpp"

#include "build_style/build_style.h"

#include <QObject>
#include <QtGlobal>
#include <QtGui/QStyleHints>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>

#include <gflags/gflags.h>

#ifdef OMIM_OS_WINDOWS
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryFile>

#include <cstdio>
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
DEFINE_string(designer, "",
              "Path to a style.mapcss under data/styles/{default|outdoors|vehicle}/{light|dark}/; starts the app in "
              "Designer mode with style-rebuilding tools, see docs/STYLES.md.");

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

#ifdef OMIM_OS_WINDOWS
void InitializeWindowsLogging(Platform const & platform)
{
  QDir const logDir(QString::fromStdString(platform.WritablePathForFile("logs")));
  if (!QDir().mkpath(logDir.path()))
    return;

  auto const timestamp = QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss-zzz");
  QString logPath;
  // QTemporaryFile::close() retains its native handle; destroy the object before the CRT reopens the path.
  {
    QTemporaryFile logFile(logDir.filePath("organicmaps-" + timestamp + "-XXXXXX.log"));
    logFile.setAutoRemove(false);
    if (!logFile.open())
    {
      LOG(LWARNING, ("Could not create log file", logFile.errorString().toStdString()));
      return;
    }
    logPath = logFile.fileName();
  }
  if (::_wfreopen(logPath.toStdWString().c_str(), L"a", stderr) == nullptr)
    return;
  // Startup writes without a console can leave cerr in a failed state even after stderr is reopened.
  std::cerr.clear();

  auto logs = logDir.entryList({"organicmaps-*.log"}, QDir::Files | QDir::NoSymLinks | QDir::CaseSensitive, QDir::Time);
  logs.removeOne(QFileInfo(logPath).fileName());
  constexpr int kPreviousLogsToKeep = 9;
  for (int i = kPreviousLogsToKeep; i < logs.size(); ++i)
    QFile::remove(logDir.filePath(logs[i]));  // Open logs are retried on a later launch.

  LOG(LINFO, ("Logging to", logPath.toStdString(), "Resources Directory:", platform.ResourcesDir(),
              "Writable Directory:", platform.WritableDir(), "Tmp Directory:", platform.TmpDir(),
              "Settings Directory:", platform.SettingsDir()));
}
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

#ifdef OMIM_OS_WINDOWS
  InitializeWindowsLogging(platform);
#endif

  LOG(LINFO, ("Organic Maps", platform.Version(), "built with QT:", QT_VERSION_STR, "runtime QT:", qVersion(),
              "detected CPU cores:", platform.CpuCores()));

  QApplication app(argc, argv);
  app.setDesktopFileName("app.organicmaps.desktop");

  QApplication::setApplicationName(FLAGS_designer.empty() ? "Organic Maps" : "Organic Maps Designer");

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
  QString mapcssFilePath;
  build_style::StyleInfo styleInfo;
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

    if (!FLAGS_designer.empty())
    {
      mapcssFilePath = QString::fromStdString(FLAGS_designer);
      if (!build_style::TryParseStyleInfo(mapcssFilePath, styleInfo))
      {
        QMessageBox::critical(nullptr, "Error",
                              QString("Could not detect map style from path:\n%1\n\n"
                                      "Expected .../styles/{default|outdoors|vehicle}/{light|dark}/style.mapcss")
                                  .arg(mapcssFilePath));
        return returnCode;
      }

      // Must be set before any style file is read, see StyleReader.
      GetStyleReader().SetDesignerMode(true);
      frameworkParams.m_fixedMapStyle = styleInfo.m_mapStyle;

      try
      {
        build_style::BuildAndApply(mapcssFilePath, styleInfo);
      }
      catch (std::exception const & e)
      {
        QMessageBox::critical(nullptr, "Error", e.what());
        return returnCode;
      }
    }

    Framework framework(frameworkParams);
    framework.SetupMeasurementSystem();
    auto const syncNightMode = [&framework]()
    {
      if (style_utils::GetNightModeSetting() == style_utils::NightMode::System)
        qt::common::ApplySystemNightMode(framework);
    };
    syncNightMode();
    qt::MainWindow w(framework, std::move(screenshotParams), QApplication::primaryScreen()->geometry(), mapcssFilePath,
                     styleInfo);
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

  if (build_style::NeedRecalculate && !mapcssFilePath.isEmpty())
  {
    try
    {
      build_style::RunRecalculationGeometryScript(mapcssFilePath, styleInfo);
    }
    catch (std::exception & e)
    {
      QMessageBox::critical(nullptr, "Error", e.what());
    }
  }

  LOG_SHORT(LINFO, ("Organic Maps finished with code", returnCode));
  return returnCode;
}
