#pragma once

#include "../base/base.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"

#include "../base/start_mem_debug.hpp"

struct XMLElement;

namespace ftype
{
  void ParseOSMTypes(char const * fPath, int scale);

  /// Get the types, name and layer fot feature with the tree of tags.
  bool GetNameAndType(XMLElement * p, vector<uint32_t> & types, string & name, int32_t & layer);
}

#include "../base/stop_mem_debug.hpp"
