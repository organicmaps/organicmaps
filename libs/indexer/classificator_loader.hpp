#pragma once

#include "indexer/map_style.hpp"

#include <string>

namespace classificator
{
// Force-(re)loads the current style's family and the outdoors family (designer live-reload relies
// on the force semantics). Other families load lazily via EnsureStyleLoaded.
void Load();

// Loads mapStyle's family (light + dark, from one decode) if it isn't loaded yet; no-op otherwise.
void EnsureStyleLoaded(MapStyle mapStyle);
bool IsStyleLoaded(MapStyle mapStyle);

// This method loads only classificator and types. It does not load and apply
// style rules. It can be used in separate modules to operate with
// number-string representations of types.
void LoadTypes(std::string const & classificatorFileStr, std::string const & typesFileStr);
}  // namespace classificator
