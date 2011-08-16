#pragma once
#include "../indexer/features_vector.hpp"

#include "../coding/reader.hpp"
#include "../coding/trie.hpp"
#include "../coding/trie_reader.hpp"

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

  class SearchInfo
  {
    FeaturesVector m_features;
    scoped_ptr<TrieIterator> m_iterator;

  public:
    SearchInfo(FilesContainerR const & cont)
      : m_features(cont),
        m_iterator(::trie::reader::ReadTrie(
                        cont.GetReader(SEARCH_INDEX_FILE_TAG),
                        trie::ValueReader(),
                        trie::EdgeValueReader()))
    {
    }

    TrieIterator * GetTrie() { return m_iterator.get(); }
    FeaturesVector * GetFeatures() { return &m_features; }
  };
}  // namespace search
