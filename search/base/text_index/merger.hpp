#pragma once

#include "search/base/text_index/reader.hpp"

class FileWriter;

namespace search_base
{
// Merges two on-disk text indexes and writes them to a new one.
class TextIndexMerger
{
public:
  // The merging process is as follows.
  // 1. Dictionaries from both indexes are read into memory, merged
  //    and written to disk.
  // 2. One uint32_t per entry is reserved in memory to calculate the
  //    offsets of the postings lists.
  // 3. One token at a time, all postings for the token are read from
  //    both indexes into memory, unified and written to disk.
  // 4. The offsets are written to disk.
  //
  // Note that the dictionary and offsets are kept in memory during the whole
  // merging process.
  static void Merge(TextIndexReader const & index1, TextIndexReader const & index2, FileWriter & sink);
};
}  // namespace search_base
