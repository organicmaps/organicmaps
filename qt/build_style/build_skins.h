#pragma once

#include <QString>

namespace build_style
{

void BuildSkins(QString const & styleDir, QString const & outputDir);
void ApplySkins(QString const & outputDir);

}  // namespace build_style
