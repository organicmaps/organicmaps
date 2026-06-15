#pragma once

#include <QtCore/QString>

namespace build_style
{
// Returns the tests' output; throws std::runtime_error with it when any test fails.
QString RunCurrentStyleTests();
}  // namespace build_style
