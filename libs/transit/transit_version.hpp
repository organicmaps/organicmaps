#pragma once

#include "coding/reader.hpp"

#include "base/assert.hpp"

#include <string>
#include <type_traits>

namespace transit
{
enum class TransitVersion
{
  OnlySubway = 0,
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
