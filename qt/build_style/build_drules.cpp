#include "build_drules.h"
#include "build_common.h"

#include "base/logging.hpp"

#include "platform/platform.hpp"

#include <exception>
#include <string>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

namespace build_style
{
void BuildDrawingRulesImpl(QString const & mapcssFile, QString const & outputDir, StyleInfo const & info)
{
  LOG(LINFO, ("Building drules from source", mapcssFile.toStdString()));
  QString const outputTemplate = JoinPathQt({outputDir, "drules_proto" + info.m_drulesSuffix});
  QString const outputFile = outputTemplate + ".bin";

  // Caller ensures that output directory is clear
  if (QFile(outputFile).exists())
    throw std::runtime_error("Output directory is not clear");

  // kothic needs the protobuf module; check upfront to give an actionable
  // error instead of a python stack trace from the middle of the build.
  try
  {
    (void)ExecProcess("python3", {"-c", "import google.protobuf"});
  }
  catch (std::exception const &)
  {
    throw std::runtime_error(
        "python3 cannot import the protobuf module. Install kothic dependencies, e.g.:\n"
        "  python3 -m pip install -r tools/kothic/requirements.txt");
  }

  // Run the script: same invocation as tools/unix/generate_drules.sh.
  (void)ExecProcess("python3", {
                                   GetExternalPath("libkomwm.py", "kothic/src", "../tools/kothic/src"),
                                   "-s",
                                   mapcssFile,
                                   "-o",
                                   outputTemplate,
                                   "-x",
                                   "-p",
                                   info.m_includeDir,
                               });

  // Ensure that generated file is not empty.
  if (QFile(outputFile).size() == 0)
    throw std::runtime_error("Drawing rules file has zero size");
}

void BuildDrawingRules(QString const & mapcssFile, QString const & outputDir, StyleInfo const & info)
{
  CopyFromDataDir("mapcss-mapping.csv", outputDir);
  CopyFromDataDir("mapcss-dynamic.txt", outputDir);
  BuildDrawingRulesImpl(mapcssFile, outputDir, info);
}

void ApplyDrawingRules(QString const & outputDir, StyleInfo const & info)
{
  // The writable dir is searched before the app resources ("wrf" scope), so
  // these copies shadow the bundled originals without touching the bundle.
  // In a dev checkout the writable dir is data/ itself — the same files
  // tools/unix/generate_drules.sh regenerates.
  QString const drulesFile = "drules_proto" + info.m_drulesSuffix + ".bin";
  CopyToWritableDir(drulesFile, outputDir);
  CopyToWritableDir("classificator.txt", outputDir);
  CopyToWritableDir("types.txt", outputDir);
  CopyToWritableDir("patterns.txt", outputDir);
  CopyToWritableDir("colors.txt", outputDir);
}
}  // namespace build_style
