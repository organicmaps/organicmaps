#pragma once

#include "../std/stdint.hpp"
#include "../std/string.hpp"

namespace osm
{

class OsmId
{
  uint64_t m_id;

public:
  /// @param[in] type "node" "way" or "relation"
  OsmId(string const & type, uint64_t osmId);
  uint64_t Id() const;
  string Type() const;
};

} // namespace osm
