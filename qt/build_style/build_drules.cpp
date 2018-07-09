#include "build_drules.h"
#include "build_common.h"

#include "platform/platform.hpp"

#include <exception>
#include <fstream>
#include <streambuf>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QStringList>

namespace
{
QString GetScriptPath()
{
  return GetExternalPath("libkomwm.py", "kothic/src", "../tools/kothic/src");
}
}  // namespace

namespace build_style
{
void BuildDrawingRulesImpl(QString const & mapcssFile, QString const & outputDir)
{
  QString const outputTemplate = JoinFoldersToPath({outputDir, "drules_proto_design"});
  QString const outputFile = outputTemplate + ".bin";

  // Caller ensures that output directory is clear
  if (QFile(outputFile).exists())
    throw std::runtime_error("Output directory is not clear");

  // Prepare command line
  QStringList params;
  params << "python" <<
            GetScriptPath() <<
            "-s" << mapcssFile <<
            "-o" << outputTemplate <<
            "-x" << "True";
  QString const cmd = params.join(' ');

  // Add path to the protobuf EGG in the PROTOBUF_EGG_PATH environment variable
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert("PROTOBUF_EGG_PATH", GetProtobufEggPath());

  // Run the script
  auto const res = ExecProcess(cmd, &env);

  // Script returs nothing and zero exit code if it is executed succesfully,
  if (res.first != 0 || !res.second.isEmpty())
  {
    QString msg = QString("System error ") + to_string(res.first).c_str();
    if (!res.second.isEmpty())
      msg = msg + "\n" + res.second;
    throw std::runtime_error(to_string(msg));
  }

  // Ensure generated files has non-zero size
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
