#pragma once

#include <QtCore/QString>
#include <QtCore/QTranslator>

#include <memory>
#include <string>

class QCoreApplication;

namespace qt
{
std::unique_ptr<QTranslator> InstallTranslator(QCoreApplication & app, std::string const & locale);
QString Tr(char const * id, int n = -1);
}  // namespace qt
