#pragma once

#include "../coding/reader.hpp"
#include "../coding/trie.hpp"
#include "../base/base.hpp"

namespace search
{
namespace trie
{

struct ValueReader
{
  struct ValueType
  {
    uint8_t m_rank;
    uint32_t m_featureId;
  };

  template <typename SourceT> void operator() (SourceT & src, ValueType & value) const
  {
    value.m_rank = ReadPrimitiveFromSource<uint8_t>(src);
    value.m_featureId = ReadPrimitiveFromSource<uint32_t>(src);
  }
};

struct EdgeValueReader
{
  typedef uint8_t ValueType;

  template <typename SourceT> void operator() (SourceT & src, ValueType & value) const
  {
    src.Read(&value, 1);
  }
};

}  // namespace search::trie

typedef ::trie::Iterator<
    search::trie::ValueReader::ValueType,
    search::trie::EdgeValueReader::ValueType> TrieIterator;

}  // namespace search
