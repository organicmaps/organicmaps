#pragma once

#include "std/cstdint.hpp"
#include "std/string.hpp"

/// Used to store and edit 3-state OSM information, for example,
/// "This place has internet", "does not have", or "it's not specified yet".
/// Explicit values are given for easier reuse in Java code.
namespace osm
{
enum YesNoUnknown
{
  Unknown = 0,
  Yes = 1,
  No = 2
};

inline string DebugPrint(YesNoUnknown value)
{
  switch (value)
  {
  case Unknown: return "Unknown";
  case Yes: return "Yes";
  case No: return "No";
  }
}
}  // namespace osm
