#pragma once

#include "qt/build_style/build_style.h"

#include <QtCore/QString>

namespace build_style
{
QString GetStyleStatistics(QString const & mapcssMappingFile, QString const & drulesFile);
QString GetCurrentStyleStatistics(StyleInfo const & info);
}  // namespace build_style
