#include "build_statistics.h"

#include "build_common.h"

#include "platform/platform.hpp"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

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

  return ExecProcess("python3", {
                                    GetExternalPath("drules_info.py", "kothic/src", "../tools/python/stylesheet"),
                                    mapcssMappingFile,
                                    drulesFile,
                                });
}

QString GetCurrentStyleStatistics(StyleInfo const & info)
{
  // Prefer the freshly built files from the writable dir over bundled ones.
  Platform const & pl = GetPlatform();
  QString const mappingPath = pl.ReadPathForFile("mapcss-mapping.csv", "wr").c_str();
  QString const drulesPath = pl.ReadPathForFile(info.m_drulesFile.toStdString(), "wr").c_str();
  return GetStyleStatistics(mappingPath, drulesPath);
}
}  // namespace build_style
