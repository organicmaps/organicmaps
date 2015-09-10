#pragma once

#include "indexer/feature_data.hpp"


struct OsmElement;

namespace ftype
{
  /// Get the types, name and layer for feature with the tree of tags.
  void GetNameAndType(OsmElement * p, FeatureParams & params);
}
