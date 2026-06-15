#pragma once

#include "indexer/map_style.hpp"

#include <string>

namespace classificator
{
// Loads types and the current/outdoors families at startup, before asynchronous readers exist.
// Other families load lazily via EnsureStyleLoaded.
void Load();

// Throws if generated type identities differ from those loaded at startup. Call before publishing
// Designer outputs: existing maps and cached type checkers cannot adopt new identities live.
void CheckTypesCompatible(std::string const & dataDir);

// Reloads drawing rules while preserving type identities, node addresses and type-selection
// priorities. GUI and rendering readers must be stopped; search uses immutable metadata.
void ReloadDrawingRules();

// Loads mapStyle's family (light + dark, from one decode) if it isn't loaded yet; no-op otherwise.
void EnsureStyleLoaded(MapStyle mapStyle);
bool IsStyleLoaded(MapStyle mapStyle);

// This method loads only classificator and types. It does not load and apply
// style rules. It can be used in separate modules to operate with
// number-string representations of types.
void LoadTypes(std::string const & classificatorFileStr, std::string const & typesFileStr);
}  // namespace classificator
