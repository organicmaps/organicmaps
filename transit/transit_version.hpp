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
}  // namespace transit
