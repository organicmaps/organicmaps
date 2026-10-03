#include "build_style.h"
#include "build_common.h"
#include "build_drules.h"
#include "build_skins.h"

#include "indexer/classificator_loader.hpp"

#include "platform/mwm_version.hpp"
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
      out.m_stylesRoot = parts.stylesRoot.absolutePath();
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

    classificator::CheckTypesCompatible(paths.m_outputDir.toStdString());
    ApplyDrawingRules(paths.m_outputDir, info);
    ApplySkins(paths.m_outputDir, info.m_theme);
  }
  else
  {
    BuildDrawingRules(paths.m_outputDir, info);
    classificator::CheckTypesCompatible(paths.m_outputDir.toStdString());
    ApplyDrawingRules(paths.m_outputDir, info);
  }
}

void RunRecalculationGeometryScript(QString const & mapcssFile, StyleInfo const & info)
{
  QString const script = GetScriptPath("recalculate_geom_index.py", "../tools/python");
  QString const generator = GetHelperPath("generator_tool");
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();
  QString const writableDir = GetPlatform().WritableDir().c_str();

  // Build Style does not rebuild the merged style that generator_tool indexes against.
  BuildMergedDrawingRules(GetStylePaths(mapcssFile).m_outputDir, info);
  PrepareEditableWorld();

  // Helpers must receive both paths before Platform construction on macOS. The script also passes
  // explicit CLI paths, so Windows can find shared edited styles independently of each map's directory.
  QProcessEnvironment env{QProcessEnvironment::systemEnvironment()};
  env.insert("MWM_RESOURCES_DIR", resourceDir);
  env.insert("MWM_WRITABLE_DIR", writableDir);

  // The trailing arguments are the relaunch command for the script.
  (void)ExecProcess(kPythonExecutable,
                    {
                        script,
                        resourceDir,
                        writableDir,
                        generator,
                        QCoreApplication::applicationFilePath(),
                        "--designer=" + mapcssFile,
                    },
                    &env);
}

void PrepareEditableWorld()
{
  auto const & platform = GetPlatform();
  auto const source = platform.ReadPathForFile("World.mwm", "r");
  auto const mapVersion = version::ReadVersionDate(ModelReaderPtr(platform.GetReader(source, "f")));
  auto const directory = JoinPathQt({QString::fromStdString(platform.WritableDir()), QString::number(mapVersion)});
  auto const destination = JoinPathQt({directory, "World.mwm"});
  if (QFileInfo::exists(destination))
    return;
  if (!QDir().mkpath(directory) || !CopyQtFile(QString::fromStdString(source), destination))
    throw std::runtime_error("Cannot prepare editable World map: " + destination.toStdString());
}

bool NeedRecalculate = false;

QString ExportPhonePackage(QString const & mapcssFile, StyleInfo const & info, QString const & targetDir,
                           std::function<bool(QString const &)> const & confirmOverwrite)
{
  auto const paths = GetStylePaths(mapcssFile);
  QString const target = QDir(targetDir).canonicalPath();
  QString const sources = QDir(info.m_stylesRoot).canonicalPath();
  if (target.isEmpty() || sources.isEmpty())
    throw std::runtime_error("The output directory or style sources do not exist");

  QString const destination = JoinPathQt({target, "styles"});
  QString const canonicalDestination = QFileInfo::exists(destination) ? QDir(destination).canonicalPath() : destination;
  auto const contains = [](QString const & parent, QString const & child)
  {
    auto const relative = QDir(parent).relativeFilePath(child);
    return !QDir::isAbsolutePath(relative) && relative != ".." && !relative.startsWith("../");
  };
  if (contains(sources, canonicalDestination) || contains(canonicalDestination, sources))
    throw std::runtime_error("Choose an output directory outside the style sources");

  QStringList files{info.m_drulesFile};
  if (paths.m_hasSymbols)
    for (auto const & dpi : kSkinDpis)
      for (char const * leaf : {"symbols.png", "symbols.xml"})
        files.push_back(JoinPathQt({"symbols", dpi.m_name, info.m_theme, leaf}));

  // Complete validation precedes both confirmation and deletion, so a missing atlas cannot erase
  // an existing package. Default-family exports require every density, including missing directories.
  for (auto const & file : files)
  {
    auto const source = JoinPathQt({paths.m_outputDir, file});
    if (!QFileInfo(source).isFile())
      throw std::runtime_error("Run Build Style first; missing " + source.toStdString());
  }

  if (QDir(destination).exists())
  {
    if (!confirmOverwrite(destination))
      return {};
    if (!QDir(destination).removeRecursively())
      throw std::runtime_error("Cannot remove existing " + destination.toStdString());
  }
  for (auto const & file : files)
  {
    auto const dest = JoinPathQt({destination, file});
    if (!QDir().mkpath(QFileInfo(dest).absolutePath()) || !CopyQtFile(JoinPathQt({paths.m_outputDir, file}), dest))
      throw std::runtime_error("Cannot export " + file.toStdString());
  }
  return destination;
}
}  // namespace build_style
