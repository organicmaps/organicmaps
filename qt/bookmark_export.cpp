#include "qt/bookmark_export.hpp"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSaveFile>

#include <array>
#include <filesystem>
#include <system_error>

namespace qt
{
bool SaveExportedFile(QString const & sourcePath, QString const & destinationPath)
{
  QFile source(sourcePath);
  if (!source.open(QIODevice::ReadOnly))
    return false;

  // The selected destination can be the temporary export itself, including a path alias.
  std::error_code error;
  if (std::filesystem::equivalent(QFileInfo(source).filesystemFilePath(),
                                  QFileInfo(destinationPath).filesystemFilePath(), error))
    return true;

  QSaveFile destination(destinationPath);
  if (!destination.open(QIODevice::WriteOnly))
    return false;

  std::array<char, 64 * 1024> buffer;
  qint64 bytesRead;
  while ((bytesRead = source.read(buffer.data(), buffer.size())) > 0)
    if (destination.write(buffer.data(), bytesRead) != bytesRead)
      return false;

  if (bytesRead < 0 || !destination.commit())
    return false;
  source.close();
  QFile::remove(sourcePath);
  return true;
}
}  // namespace qt
