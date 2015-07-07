#pragma once

#include "indexer/feature_data.hpp"


struct XMLElement;

namespace ftype
{
  /// Get the types, name and layer for feature with the tree of tags.
  void GetNameAndType(XMLElement * p, FeatureParams & params);

  /// Check that final types set for feature is valid.
  bool IsValidTypes(FeatureParams const & params);
}
