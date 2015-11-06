#pragma once

#include "std/string.hpp"

class FilesContainerR;
class Writer;

namespace indexer
{
bool BuildSearchIndexFromDataFile(string const & filename, bool forceRebuild = false);

void BuildSearchIndex(FilesContainerR & container, Writer & indexWriter);
}  // namespace indexer
