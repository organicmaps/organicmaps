#include "build_drules.h"
#include "build_common.h"

#include "base/logging.hpp"

#include <stdexcept>
#include <string>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QLatin1String>

namespace build_style
{
namespace
{
// Builds one style.mapcss variant into <outputDir>/<baseName>.bin with the same libkomwm.py
// invocation as tools/unix/generate_drules.sh. The -d data path is the output dir, so the side
// files (classificator.txt, types.txt, colors.txt, patterns.txt) are written there and accumulate
// across both variant builds, exactly like the bundled generation.
void BuildVariant(QString const & mapcssFile, QString const & outputDir, QString const & baseName,
                  QString const & includeDir)
{
  LOG(LINFO, ("Building drules from source", mapcssFile.toStdString()));
  QString const outputTemplate = JoinPathQt({outputDir, baseName});
  QString const outputFile = outputTemplate + ".bin";

  (void)ExecProcess("python3", {
                                   GetExternalPath("libkomwm.py", "../tools/kothic/src"),
                                   "-s",
                                   mapcssFile,
                                   "-o",
                                   outputTemplate,
                                   "-p",
                                   includeDir,
                                   "-d",
                                   outputDir,
                               });

  // QFile::size() is also 0 when the file was never created.
  if (QFile(outputFile).size() == 0)
    throw std::runtime_error("Drawing rules file was not created or is empty: " + outputFile.toStdString());
}

void BuildDrawingRulesImpl(QString const & outputDir, StyleInfo const & info)
{
  // The native reader loads a packed family file (drules_<family>.bin) with the light variant at
  // index 0 and dark at index 1, so rebuild BOTH variants of the edited family and pack them, just
  // like tools/unix/generate_drules.sh. The edited theme is built last so the accumulated side
  // files (colors.txt, patterns.txt, ...) match the variant being previewed.
  QString const lightBase = info.m_styleType + "_light";
  QString const darkBase = info.m_styleType + "_dark";

  if (info.m_theme == QLatin1String("dark"))
  {
    BuildVariant(info.m_lightMapcss, outputDir, lightBase, info.m_includeDir);
    BuildVariant(info.m_darkMapcss, outputDir, darkBase, info.m_includeDir);
  }
  else
  {
    BuildVariant(info.m_darkMapcss, outputDir, darkBase, info.m_includeDir);
    BuildVariant(info.m_lightMapcss, outputDir, lightBase, info.m_includeDir);
  }

  // Pack light (variant 0) and dark (variant 1) into the real family file the app reads.
  QString const familyTemplate = JoinPathQt({outputDir, "drules_" + info.m_styleType});
  (void)ExecProcess("python3", {
                                   GetExternalPath("merge_variants.py", "../tools/kothic/src"),
                                   familyTemplate,
                                   "light",
                                   JoinPathQt({outputDir, lightBase + ".bin"}),
                                   "dark",
                                   JoinPathQt({outputDir, darkBase + ".bin"}),
                               });

  QString const familyFile = familyTemplate + ".bin";
  if (QFile(familyFile).size() == 0)
    throw std::runtime_error("Packed drawing rules file was not created or is empty: " + familyFile.toStdString());
}
}  // namespace

void BuildDrawingRules(QString const & outputDir, StyleInfo const & info)
{
  CopyFromDataDir("mapcss-mapping.csv", outputDir);
  CopyFromDataDir("mapcss-dynamic.txt", outputDir);
  // libkomwm accumulates colors and patterns into these two files rather than rewriting them, and
  // generate_drules.sh deletes them once before building all six variants. Only one family is
  // rebuilt here, so seed them with the full bundled set: otherwise ApplyDrawingRules() would
  // overwrite the writable dir's files with a subset, dropping the other families' colors and the
  // transit ones that generate_drules.sh appends.
  CopyFromDataDir("colors.txt", outputDir);
  CopyFromDataDir("patterns.txt", outputDir);
  BuildDrawingRulesImpl(outputDir, info);
}

void BuildMergedDrawingRules(QString const & outputDir, StyleInfo const & info)
{
  // generator_tool indexes every map against the merged style, the zoom-range union of the three
  // light styles, which only tools/unix/generate_drules.sh produces. Rebuild it the same way: the
  // edited family's light variant is already in outputDir, compile the other two families' next to
  // it and merge them in the script's order, which the result depends on.
  auto const lightVariant = [&outputDir](QString const & family)
  { return JoinPathQt({outputDir, family + "_light.bin"}); };

  for (QString const family : {"default", "vehicle", "outdoors"})
  {
    if (family == info.m_styleType)
      continue;
    QDir const familyDir(JoinPathQt({info.m_stylesRoot, family}));
    BuildVariant(familyDir.absoluteFilePath("light/style.mapcss"), outputDir, family + "_light",
                 familyDir.absoluteFilePath("include") + QDir::separator());
  }

  QString const mergedFile = JoinPathQt({outputDir, "drules_merged.bin"});
  (void)ExecProcess("python3", {
                                   GetExternalPath("merge_styles.py", "../tools/kothic/src"),
                                   lightVariant("default"),
                                   lightVariant("vehicle"),
                                   lightVariant("outdoors"),
                                   mergedFile,
                               });
  if (QFile(mergedFile).size() == 0)
    throw std::runtime_error("Merged drawing rules file was not created or is empty: " + mergedFile.toStdString());

  CopyToWritableDir("drules_merged.bin", outputDir);
}

void ApplyDrawingRules(QString const & outputDir, StyleInfo const & info)
{
  // The writable dir is searched before the app resources ("wrf" scope), so these copies shadow
  // the bundled originals without touching the bundle. In a dev checkout the writable dir is data/
  // itself — the same files tools/unix/generate_drules.sh regenerates.
  CopyToWritableDir(info.m_drulesFile, outputDir);
  CopyToWritableDir("classificator.txt", outputDir);
  CopyToWritableDir("types.txt", outputDir);
  CopyToWritableDir("patterns.txt", outputDir);
  CopyToWritableDir("colors.txt", outputDir);
}
}  // namespace build_style
