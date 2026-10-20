#pragma once

#include <QtCore/QLocale>
#include <QtCore/QString>
#include <QtCore/QTranslator>

#include <memory>
#include <span>
#include <string>

class QApplication;

namespace qt
{
std::unique_ptr<QTranslator> InstallTranslator(QApplication & app, std::span<std::string const> preferred);
// Unit names follow the selected catalog; compatible regional formatting is retained.
QLocale GetDataSizeLocale(QTranslator const & translator, QLocale const & regionalLocale = QLocale());
QString Tr(char const * id, int n = -1);
}  // namespace qt
