#pragma once

#include "std/string.hpp"

class FilesContainerR;
class Writer;

namespace indexer
{
bool BuildSearchIndexFromDatFile(string const & fName, bool forceRebuild = false);

bool AddCompresedSearchIndexSection(string const & fName, bool forceRebuild);

void BuildCompressedSearchIndex(FilesContainerR & container, Writer & indexWriter);

void BuildCompressedSearchIndex(string const & fName, Writer & indexWriter);
}  // namespace indexer
