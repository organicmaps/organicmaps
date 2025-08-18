#include "build_statistics.h"

#include "build_common.h"

#include "platform/platform.hpp"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcessEnvironment>

#include <exception>
#include <string>

namespace build_style
{
QString GetStyleStatistics(QString const & mapcssMappingFile, QString const & drulesFile)
{
  if (!QFile(mapcssMappingFile).exists())
    throw std::runtime_error("mapcss-mapping file does not exist at " + mapcssMappingFile.toStdString());

  if (!QFile(drulesFile).exists())
    throw std::runtime_error("drawing-rules file does not exist at " + drulesFile.toStdString());

  // Add path to the protobuf EGG in the PROTOBUF_EGG_PATH environment variable.
  QProcessEnvironment env{QProcessEnvironment::systemEnvironment()};
  env.insert("PROTOBUF_EGG_PATH", GetProtobufEggPath());

  // Run the script.
  return ExecProcess("python",
                     {
                         GetExternalPath("drules_info.py", "kothic/src", "../tools/python/stylesheet"),
                         mapcssMappingFile,
                         drulesFile,
                     },
                     &env);
}

QString GetCurrentStyleStatistics()
{
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();
  QString const mappingPath = JoinPathQt({resourceDir, "mapcss-mapping.csv"});
  QString const drulesPath = JoinPathQt({resourceDir, "drules_proto_design.bin"});
  return GetStyleStatistics(mappingPath, drulesPath);
}
}  // namespace build_style
