#pragma once

#include "std/string.hpp"

class FilesContainerR;
class Writer;

namespace indexer
{
// Builds the latest version of the centers table section and writes
// it to the mwm file.
bool BuildCentersTableFromDataFile(string const & filename, bool forceRebuild = false);
}  // namespace indexer
