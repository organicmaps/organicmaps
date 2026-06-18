#include "build_style.h"
#include "build_common.h"
#include "build_drules.h"
#include "build_skins.h"

#include "platform/platform.hpp"

#include <exception>
#include <future>
#include <string>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcessEnvironment>

namespace
{
QString GetRecalculateGeometryScriptPath()
{
  return GetExternalPath("recalculate_geom_index.py", "", "../tools/python");
}

QString GetGeometryToolPath()
{
  return GetExternalPath("generator_tool", "generator_tool.app/Contents/MacOS", "");
}

struct StylePathParts
{
  QString styleType;
  QString theme;
  QDir stylesRoot;  // .../styles/
};

bool SplitStylePath(QString const & mapcssFile, StylePathParts & out)
{
  // Expecting <stylesRoot>/<type>/<theme>/style.mapcss.  Walk up three levels
  // (file -> theme -> type -> stylesRoot) and capture the segment names.
  QFileInfo const fi(mapcssFile);
  if (fi.fileName() != "style.mapcss")
    return false;

  QDir themeDir = fi.absoluteDir();
  out.theme = themeDir.dirName();

  QDir typeDir = themeDir;
  if (!typeDir.cdUp())
    return false;
  out.styleType = typeDir.dirName();

  QDir rootDir = typeDir;
  if (!rootDir.cdUp())
    return false;
  out.stylesRoot = rootDir;
  return true;
}

struct StyleEntry
{
  char const * styleType;
  char const * theme;
  MapStyle mapStyle;
};

// Mirrors the (type, theme) layout under data/styles/.
StyleEntry const kSupportedStyles[] = {
    {"default", "light", MapStyleDefaultLight},   {"default", "dark", MapStyleDefaultDark},
    {"outdoors", "light", MapStyleOutdoorsLight}, {"outdoors", "dark", MapStyleOutdoorsDark},
    {"vehicle", "light", MapStyleVehicleLight},   {"vehicle", "dark", MapStyleVehicleDark},
};

struct StylePaths
{
  QString m_styleDir;   // Directory of style.mapcss, with a trailing separator.
  QString m_outputDir;  // <styleDir>/out/, with a trailing separator.
  bool m_hasSymbols;    // Only default/{light,dark} carry their own symbols/ sources.
};

StylePaths GetStylePaths(QString const & mapcssFile)
{
  if (!QFile(mapcssFile).exists())
    throw std::runtime_error("mapcss file does not exist: " + mapcssFile.toStdString());

  QString const styleDir = QFileInfo(mapcssFile).absolutePath() + QDir::separator();
  return {styleDir, styleDir + "out" + QDir::separator(), QDir(styleDir + "symbols/").exists()};
}
}  // namespace

namespace build_style
{
bool TryParseStyleInfo(QString const & mapcssFile, StyleInfo & out)
{
  StylePathParts parts;
  if (!SplitStylePath(mapcssFile, parts))
    return false;

  for (auto const & e : kSupportedStyles)
  {
    if (parts.styleType == QLatin1String(e.styleType) && parts.theme == QLatin1String(e.theme))
    {
      out.m_mapStyle = e.mapStyle;
      out.m_styleType = parts.styleType;
      out.m_theme = parts.theme;
      // The native reader loads a packed family file shared by light and dark; its name matches the
      // drules_<family>.bin naming of map_style_reader.cpp (family == style type here).
      out.m_drulesFile = "drules_" + parts.styleType + ".bin";
      out.m_includeDir =
          parts.stylesRoot.absoluteFilePath(parts.styleType + QDir::separator() + "include") + QDir::separator();
      // Both variants are rebuilt and packed on every Build Style, so capture both mapcss paths.
      out.m_lightMapcss = parts.stylesRoot.absoluteFilePath(parts.styleType + "/light/style.mapcss");
      out.m_darkMapcss = parts.stylesRoot.absoluteFilePath(parts.styleType + "/dark/style.mapcss");
      return true;
    }
  }
  return false;
}

void BuildAndApply(QString const & mapcssFile, StyleInfo const & info)
{
  auto const paths = GetStylePaths(mapcssFile);

  // Ensure output directory is clear
  if (QDir(paths.m_outputDir).exists() && !QDir(paths.m_outputDir).removeRecursively())
    throw std::runtime_error("Unable to remove the output directory");
  if (!QDir().mkdir(paths.m_outputDir))
    throw std::runtime_error("Unable to make the output directory");

  if (paths.m_hasSymbols)
  {
    auto future = std::async(std::launch::async, BuildSkins, paths.m_styleDir, paths.m_outputDir, info.m_theme);
    BuildDrawingRules(paths.m_outputDir, info);
    future.get();  // may rethrow exception from the BuildSkin

    ApplyDrawingRules(paths.m_outputDir, info);
    ApplySkins(paths.m_outputDir, info.m_theme);
  }
  else
  {
    BuildDrawingRules(paths.m_outputDir, info);
    ApplyDrawingRules(paths.m_outputDir, info);
  }
}

void BuildIfNecessaryAndApply(QString const & mapcssFile, StyleInfo const & info)
{
  auto const paths = GetStylePaths(mapcssFile);

  if (QDir(paths.m_outputDir).exists())
  {
    try
    {
      ApplyDrawingRules(paths.m_outputDir, info);
      if (paths.m_hasSymbols)
        ApplySkins(paths.m_outputDir, info.m_theme);
    }
    catch (std::exception const &)
    {
      BuildAndApply(mapcssFile, info);
    }
  }
  else
  {
    BuildAndApply(mapcssFile, info);
  }
}

void RunRecalculationGeometryScript(QString const & mapcssFile)
{
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();
  QString const writableDir = GetPlatform().WritableDir().c_str();

  // generator_tool must load the classificator and drules exactly as this app
  // sees them, including the freshly built files in the writable dir. Both
  // Platform implementations honour these variables (see Platform() ctors),
  // so no files have to be copied around.
  QProcessEnvironment env{QProcessEnvironment::systemEnvironment()};
  env.insert("MWM_RESOURCES_DIR", resourceDir);
  env.insert("MWM_WRITABLE_DIR", writableDir);

  // The trailing arguments are the relaunch command for the script.
  (void)ExecProcess("python3",
                    {
                        GetRecalculateGeometryScriptPath(),
                        resourceDir,
                        writableDir,
                        GetGeometryToolPath(),
                        QCoreApplication::applicationFilePath(),
                        "--designer=" + mapcssFile,
                    },
                    &env);
}

bool NeedRecalculate = false;
}  // namespace build_style
