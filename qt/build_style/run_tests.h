#pragma once

#include <QtCore/QString>

#include <utility>

namespace build_style
{
std::pair<bool, QString> RunCurrentStyleTests();
} // namespace build_style
