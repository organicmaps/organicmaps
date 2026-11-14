#pragma once

#include <QString>

namespace sailfish::file_log
{
// Core and Qt debug logs in a file, for bug reports.
void Enable(bool enabled);
bool IsEnabled();
QString Path();
}  // namespace sailfish::file_log
