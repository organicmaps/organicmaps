#pragma once

#include <QtCore/QString>

namespace build_style
{
void BuildAndApply(QString const & mapcssFile);
void RunRecalculationGeometryScript(QString const & mapcssFile);

extern bool NeedRecalculate;
}  // namespace build_style
