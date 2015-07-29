#pragma once

#include "indexer/geometry_serialization.hpp"

#include "coding/reader.hpp"
#include "coding/trie.hpp"
#include "coding/trie_reader.hpp"


namespace search
{
static const uint8_t kCategoriesLang = 128;
static const uint8_t kPointCodingBits = 20;
}  // namespace search

namespace trie
{

// Value: feature offset and search rank are stored.
class ValueReader
{
  serial::CodingParams const & m_cp;

public:
  explicit ValueReader(serial::CodingParams const & cp) : m_cp(cp) {}

  struct ValueType
  {
    m2::PointD m_pt;        // Center point of feature;
    uint32_t m_featureId;   // Offset of the feature;
    uint8_t m_rank;         // Rank of feature;
  };

  template <typename TSource>
  void operator()(TSource & src, ValueType & v) const
  {
    v.m_pt = serial::LoadPoint(src, m_cp);
    v.m_featureId = ReadPrimitiveFromSource<uint32_t>(src);
    v.m_rank = ReadPrimitiveFromSource<uint8_t>(src);
  }

  template <class TSink>
  void Save(TSink & sink, ValueType const & v) const
  {
    serial::SavePoint(sink, v.m_pt, m_cp);
    WriteToSink(sink, v.m_featureId);
    WriteToSink(sink, v.m_rank);
  }
};

using TEdgeValueReader = EmptyValueReader;
using DefaultIterator =
    trie::Iterator<trie::ValueReader::ValueType, trie::TEdgeValueReader::ValueType>;

inline serial::CodingParams GetCodingParams(serial::CodingParams const & orig)
{
  return serial::CodingParams(search::kPointCodingBits,
                              PointU2PointD(orig.GetBasePoint(), orig.GetCoordBits()));
}

}  // namespace trie
