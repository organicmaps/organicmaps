#pragma once

#include "geometry/rect2d.hpp"

#include "base/base.hpp"

struct FeatureID;
class StringUtf8Multilang;

namespace feature
{
  class TypesHolder;

  /// Get viewport scale to show given feature. Used in search.
  int GetFeatureViewportScale(TypesHolder const & types);

  void GetPreferredNames(FeatureID const & id, StringUtf8Multilang const & src, string & primary,
                         string & secondary);
  void GetReadableName(FeatureID const & id, StringUtf8Multilang const & src, string & out);
}  // namespace feature
