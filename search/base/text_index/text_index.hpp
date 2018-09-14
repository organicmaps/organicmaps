#pragma once

#include <cstdint>
#include <string>

// This file contains the structures needed to store an
// updatable text index on disk.
//
// The index maps tokens of string type (typically std::string or
// strings::UniString) to postings lists, i.e. to lists of entities
// called postings that encode the locations of the strings in the collection
// of the text documents that is being indexed. An example of a posting
// is a document id (docid). Another example is a pair of a document id and
// a position within the corresponding document.
//
// The updates are performed by rebuilding the index, either as a result
// of merging several indexes together, or as a result of clearing outdated
// entries from an old index.
//
// For version 0, the postings lists are docid arrays, i.e. arrays of unsigned
// 32-bit integers stored in increasing order.
// The structure of the index is:
//   [header: version and offsets]
//   [array containing the starting positions of tokens]
//   [tokens, written without separators in the lexicographical order]
//   [array containing the offsets for the postings lists]
//   [postings lists, stored as delta-encoded varints]
//
// All offsets are measured relative to the start of the index.
namespace search_base
{
using Token = std::string;
using Posting = uint32_t;

enum class TextIndexVersion : uint8_t
{
  V0 = 0,
  Latest = V0
};

std::string DebugPrint(TextIndexVersion const & version);
}  // namespace search_base
