#include "build_drules.h"
#include "build_common.h"

#include "platform/platform.hpp"

#include <exception>
#include <fstream>
#include <streambuf>
#include <string>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcessEnvironment>

namespace build_style
{
void BuildDrawingRulesImpl(QString const & mapcssFile, QString const & outputDir)
{
  QString const outputTemplate = JoinPathQt({outputDir, "drules_proto_design"});
  QString const outputFile = outputTemplate + ".bin";

  // Caller ensures that output directory is clear
  if (QFile(outputFile).exists())
    throw std::runtime_error("Output directory is not clear");

  // Add path to the protobuf EGG in the PROTOBUF_EGG_PATH environment variable
  QProcessEnvironment env{QProcessEnvironment::systemEnvironment()};
  env.insert("PROTOBUF_EGG_PATH", GetProtobufEggPath());

  // Run the script
  (void)ExecProcess("python",
                    {
                        GetExternalPath("libkomwm.py", "kothic/src", "../tools/kothic/src"),
                        "-s",
                        mapcssFile,
                        "-o",
                        outputTemplate,
                        "-x",
                        "True",
                    },
                    &env);

  // Ensure that generated file is not empty.
  if (QFile(outputFile).size() == 0)
    throw std::runtime_error("Drawing rules file has zero size");
}

void BuildDrawingRules(QString const & mapcssFile, QString const & outputDir)
{
  CopyFromResources("mapcss-mapping.csv", outputDir);
  CopyFromResources("mapcss-dynamic.txt", outputDir);
  BuildDrawingRulesImpl(mapcssFile, outputDir);
}

void ApplyDrawingRules(QString const & outputDir)
{
  CopyToResources("drules_proto_design.bin", outputDir);
  CopyToResources("classificator.txt", outputDir);
  CopyToResources("types.txt", outputDir);
  CopyToResources("patterns.txt", outputDir, "patterns_design.txt");
  CopyToResources("colors.txt", outputDir, "colors_design.txt");
}
}  // namespace build_style
