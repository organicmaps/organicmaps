#pragma once

#include <QString>

namespace build_style
{

void BuildDrawingRules(QString const & mapcssFile, QString const & outputDir);
void ApplyDrawingRules(QString const & outputDir);

}  // build_style
