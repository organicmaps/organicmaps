#pragma once

#include <QtCore/QString>

namespace build_style
{
// `theme` is "light" or "dark"; selects the symbols/<dpi>/<theme>/ output dir.
void BuildSkins(QString const & styleDir, QString const & outputDir, QString const & theme);
void ApplySkins(QString const & outputDir, QString const & theme);
}  // namespace build_style
