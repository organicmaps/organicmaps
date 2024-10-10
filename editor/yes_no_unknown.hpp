#pragma once

#include <string>

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
}  // namespace osm
