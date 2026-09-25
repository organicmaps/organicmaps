#pragma once

#include <QtCore/QString>

namespace qt
{
// Saves atomically and removes the temporary source only when it is a different file.
bool SaveExportedFile(QString const & sourcePath, QString const & destinationPath);
}  // namespace qt
