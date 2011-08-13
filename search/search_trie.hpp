#pragma once

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
    uint32_t m_FeatureId;
    uint8_t m_Rank;
  };

  template <typename SourceT> void operator() (SourceT & src, ValueType & value) const
  {
    src.Read(&value, 5);
  }
};

struct EdgeValueReader
{
  struct ValueType
  {
    uint8_t m_Rank;
  };

  template <typename SourceT> void operator() (SourceT & src, ValueType & value) const
  {
    src.Read(&value.m_Rank, 1);
  }
};

}  // namespace search::trie

typedef ::trie::Iterator<search::trie::ValueReader, search::trie::EdgeValueReader> TrieIterator;

}  // namespace search
