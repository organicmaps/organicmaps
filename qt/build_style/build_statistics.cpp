#include "build_statistics.h"

#include "build_common.h"

#include "platform/platform.hpp"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QStringList>

#include <exception>

namespace
{
QString GetScriptPath()
{
  return GetExternalPath("drules_info.py", "kothic/src", "../tools/python/stylesheet");
}
}  // namespace

namespace build_style
{

QString GetStyleStatistics(QString const & mapcssMappingFile, QString const & drulesFile)
{
  if (!QFile(mapcssMappingFile).exists())
    throw std::runtime_error("mapcss-mapping file does not exist");

  if (!QFile(drulesFile).exists())
    throw std::runtime_error("drawing-rules file does not exist");

  // Prepare command line
  QStringList params;
  params << "python" << GetScriptPath() << mapcssMappingFile << drulesFile;
  QString const cmd = params.join(' ');

  // Add path to the protobuf EGG in the PROTOBUF_EGG_PATH environment variable
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert("PROTOBUF_EGG_PATH", GetProtobufEggPath());

  // Run the script
  auto const res = ExecProcess(cmd, &env);

  QString text;
  if (res.first != 0)
  {
    text = QString("System error ") + to_string(res.first).c_str();
    if (!res.second.isEmpty())
      text = text + "\n" + res.second;
  }
  else
    text = res.second;

  return text;
}

QString GetCurrentStyleStatistics()
{
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();
  QString const mappingPath = JoinFoldersToPath({resourceDir, "mapcss-mapping.csv"});
  QString const drulesPath = JoinFoldersToPath({resourceDir, "drules_proto_design.bin"});
  return GetStyleStatistics(mappingPath, drulesPath);
}
}  // namespace build_style

