#include "build_statistics.h"

#include "build_common.h"

#include "std/exception.hpp"

#include "platform/platform.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>

namespace
{

QString GetScriptPath()
{
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();
  return resourceDir + "kothic/src/drules_info.py";
}

QString GetProtobufEggPath()
{
  QString const resourceDir = GetPlatform().ResourcesDir().c_str();
  return resourceDir + "kothic/protobuf-2.6.1-py2.7.egg";
}

}  // namespace

namespace build_style
{

QString GetStyleStatistics(QString const & mapcssMappingFile, QString const & drulesFile)
{
  if (!QFile(mapcssMappingFile).exists())
    throw runtime_error("mapcss-mapping file does not exist");

  if (!QFile(drulesFile).exists())
    throw runtime_error("drawing-rules file does not exist");

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
  QString const mappingPath = resourceDir + "mapcss-mapping.csv";
  QString const drulesPath = resourceDir + "drules_proto.bin";
  return GetStyleStatistics(mappingPath, drulesPath);
}

}  // namespace build_style

