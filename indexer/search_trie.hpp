#pragma once

#include "../coding/reader.hpp"
#include "../coding/trie.hpp"
#include "../coding/trie_reader.hpp"


namespace search
{
namespace trie
{

// Value: feature offset and search rank are stored.
struct ValueReader
{
  struct ValueType
  {
    uint32_t m_featureId;  // Offset of the featuer.
  };

  template <typename SourceT> void operator() (SourceT & src, ValueType & value) const
  {
    value.m_featureId = ReadPrimitiveFromSource<uint32_t>(src);
  }
};

typedef ::trie::reader::EmptyValueReader EdgeValueReader;

}  // namespace search::trie

  typedef ::trie::Iterator<
      search::trie::ValueReader::ValueType,
      search::trie::EdgeValueReader::ValueType> TrieIterator;

  static const uint8_t CATEGORIES_LANG = 128;

}  // namespace search
