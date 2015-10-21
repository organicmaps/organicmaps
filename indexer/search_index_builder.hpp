#pragma once

#include "std/string.hpp"

class FilesContainerR;
class Writer;

namespace indexer
{
bool BuildSearchIndexFromDatFile(string const & filename, bool forceRebuild = false);

void BuildSearchIndex(FilesContainerR & container, Writer & indexWriter,
                      string const & stringsFilePath);
}  // namespace indexer
