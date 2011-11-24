#pragma once

#include "../indexer/feature_data.hpp"


struct XMLElement;

namespace ftype
{
  /// Get the types, name and layer for feature with the tree of tags.
  bool GetNameAndType(XMLElement * p, FeatureParams & params);
}
