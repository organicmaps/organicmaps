#pragma once

#include "../indexer/feature_data.hpp"

#include "../base/start_mem_debug.hpp"

struct XMLElement;

namespace ftype
{
  void ParseOSMTypes(char const * fPath, int scale);

  /// Get the types, name and layer for feature with the tree of tags.
  bool GetNameAndType(XMLElement * p, FeatureParams & params);
}

#include "../base/stop_mem_debug.hpp"
