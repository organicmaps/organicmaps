#pragma once

#include <QString>

namespace sailfish::file_log
{
// Core and Qt debug logs in a file, for bug reports.
// Installs the core log, assert and Qt message handlers, before other threads log.
void Init(bool enabled);
void Enable(bool enabled);
bool IsEnabled();
QString Path();
}  // namespace sailfish::file_log
