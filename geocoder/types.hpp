#pragma once

#include "base/string_utils.hpp"

#include <string>
#include <vector>

namespace geocoder
{
using Tokens = std::vector<std::string>;

enum class Type
{
  // It is important that the types are ordered from
  // the more general to the more specific.
  Country,
  Region,
  Subregion,
  Locality,
  Suburb,
  Sublocality,
  Street,
  Building,

  Count
};

std::string ToString(Type type);
std::string DebugPrint(Type type);
}  // namespace geocoder
