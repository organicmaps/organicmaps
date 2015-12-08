#pragma once

#include <QString>

namespace build_style
{

QString GetStyleStatistics(QString const & mapcssMappingFile, QString const & drulesFile);

QString GetCurrentStyleStatistics();

} // namespace build_style
