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

QString GetGeometryToolResourceDir()
{
  return GetExternalPath("", "generator_tool.app/Contents/Resources", "");
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
  char const * drulesSuffix;
};

// Mirrors the (type, theme) layout under data/styles/ and the suffixes used
// by libs/indexer/map_style_reader.cpp.
StyleEntry const kSupportedStyles[] = {
    {"default", "light", MapStyleDefaultLight, "_default_light"},
    {"default", "dark", MapStyleDefaultDark, "_default_dark"},
    {"outdoors", "light", MapStyleOutdoorsLight, "_outdoors_light"},
    {"outdoors", "dark", MapStyleOutdoorsDark, "_outdoors_dark"},
    {"vehicle", "light", MapStyleVehicleLight, "_vehicle_light"},
    {"vehicle", "dark", MapStyleVehicleDark, "_vehicle_dark"},
};
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
      out.m_drulesSuffix = QLatin1String(e.drulesSuffix);
      out.m_includeDir =
          parts.stylesRoot.absoluteFilePath(parts.styleType + QDir::separator() + "include") + QDir::separator();
      return true;
    }
  }
  return false;
}

void BuildAndApply(QString const & mapcssFile, StyleInfo const & info)
{
  // Ensure mapcss exists
  if (!QFile(mapcssFile).exists())
    throw std::runtime_error("mapcss files does not exist");

  QDir const projectDir = QFileInfo(mapcssFile).absoluteDir();
  QString const styleDir = projectDir.absolutePath() + QDir::separator();
  QString const outputDir = styleDir + "out" + QDir::separator();

  // Ensure output directory is clear
  if (QDir(outputDir).exists() && !QDir(outputDir).removeRecursively())
    throw std::runtime_error("Unable to remove the output directory");
  if (!QDir().mkdir(outputDir))
    throw std::runtime_error("Unable to make the output directory");

  bool const hasSymbols = QDir(styleDir + "symbols/").exists();
  if (hasSymbols)
  {
    auto future = std::async(std::launch::async, BuildSkins, styleDir, outputDir, info.m_theme);
    BuildDrawingRules(mapcssFile, outputDir, info);
    future.get();  // may rethrow exception from the BuildSkin

    ApplyDrawingRules(outputDir, info);
    ApplySkins(outputDir, info.m_theme);
  }
  else
  {
    BuildDrawingRules(mapcssFile, outputDir, info);
    ApplyDrawingRules(outputDir, info);
  }
}

void BuildIfNecessaryAndApply(QString const & mapcssFile, StyleInfo const & info)
{
  if (!QFile(mapcssFile).exists())
    throw std::runtime_error("mapcss files does not exist");

  QDir const projectDir = QFileInfo(mapcssFile).absoluteDir();
  QString const styleDir = projectDir.absolutePath() + QDir::separator();
  QString const outputDir = styleDir + "out" + QDir::separator();

  if (QDir(outputDir).exists())
  {
    try
    {
      ApplyDrawingRules(outputDir, info);
      ApplySkins(outputDir, info.m_theme);
    }
    catch (std::exception const & ex)
    {
      BuildAndApply(mapcssFile, info);
    }
  }
  else
  {
    BuildAndApply(mapcssFile, info);
  }
}

void RunRecalculationGeometryScript(QString const & mapcssFile, StyleInfo const & info)
{
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();
  QString const writableDir = GetPlatform().WritableDir().c_str();

  QString const generatorToolPath = GetGeometryToolPath();
  QString const appPath = QCoreApplication::applicationFilePath();

  QString const geometryToolResourceDir = GetGeometryToolResourceDir();

  QString const drulesFile = "drules_proto" + info.m_drulesSuffix + ".bin";
  CopyFromResources(drulesFile, geometryToolResourceDir);
  CopyFromResources("classificator.txt", geometryToolResourceDir);
  CopyFromResources("types.txt", geometryToolResourceDir);

  (void)ExecProcess("python3", {
                                   GetRecalculateGeometryScriptPath(),
                                   resourceDir,
                                   writableDir,
                                   generatorToolPath,
                                   appPath,
                                   mapcssFile,
                               });
}

bool NeedRecalculate = false;
}  // namespace build_style
