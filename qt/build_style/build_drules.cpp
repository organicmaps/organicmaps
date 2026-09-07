#include "build_drules.h"
#include "build_common.h"

#include "base/logging.hpp"

#include <stdexcept>
#include <string>

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
                                   GetExternalPath("libkomwm.py", "kothic/src", "../tools/kothic/src"),
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
}  // namespace

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
                                   GetExternalPath("merge_variants.py", "kothic/src", "../tools/kothic/src"),
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

void BuildDrawingRules(QString const & outputDir, StyleInfo const & info)
{
  CopyFromDataDir("mapcss-mapping.csv", outputDir);
  CopyFromDataDir("mapcss-dynamic.txt", outputDir);
  BuildDrawingRulesImpl(outputDir, info);
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
