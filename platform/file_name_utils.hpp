#pragma once

#include "../std/string.hpp"

namespace pl
{
  /// Remove extension from file name.
  void GetNameWithoutExt(string & name);

  /// Get file name from download url.
  void GetNameFromURLRequest(string & name);

  /// Get file name from full path.
  void GetNameFromPath(string & name);
}
