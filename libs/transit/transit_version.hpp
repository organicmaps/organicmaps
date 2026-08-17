#pragma once

#include "coding/reader.hpp"

#include "base/assert.hpp"

#include <string>
#include <type_traits>

namespace transit
{
enum class TransitVersion
{
  // V1 transit section (TRANSIT_FILE_TAG, routing::transit::GraphData). The name is legacy: it used
  // to carry metro only, but now holds all OSM route-relation transit (subway, bus, tram, trolleybus).
  // Treat it as "V1 format", not "subway-only".
  OnlySubway = 0,
  // V2 experimental section (GTFS-based, ::transit::experimental::TransitData).
  AllPublicTransport = 1,
  Counter = 2
};

// Reads version from header in the transit mwm section and returns it.
TransitVersion GetVersion(Reader & reader);

inline std::string DebugPrint(TransitVersion version)
{
  switch (version)
  {
  case TransitVersion::OnlySubway: return "OnlySubway";
  case TransitVersion::AllPublicTransport: return "AllPublicTransport";
  case TransitVersion::Counter: return "Counter";
  }

  LOG(LERROR, ("Unknown version:", static_cast<std::underlying_type<TransitVersion>::type>(version)));
  UNREACHABLE();
}
}  // namespace transit
