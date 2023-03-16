#pragma once

#include <string>

class FilesContainerR;
class Writer;

namespace indexer
{
// Builds the latest version of the centers table section and writes
// it to the mwm file.
bool BuildCentersTableFromDataFile(std::string const & filename, bool forceRebuild = false);
}  // namespace indexer
