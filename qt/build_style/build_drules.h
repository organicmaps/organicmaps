#pragma once

#include "qt/build_style/build_style.h"

#include <QtCore/QString>

namespace build_style
{
void BuildDrawingRules(QString const & outputDir, StyleInfo const & info);
void ApplyDrawingRules(QString const & outputDir, StyleInfo const & info);
}  // namespace build_style
