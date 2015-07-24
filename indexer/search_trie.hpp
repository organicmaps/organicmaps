#pragma once

#include "indexer/geometry_serialization.hpp"

#include "coding/reader.hpp"
#include "coding/trie.hpp"
#include "coding/trie_reader.hpp"


namespace search
{
static const uint8_t CATEGORIES_LANG = 128;
static const uint8_t POINT_CODING_BITS = 20;
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

  template <typename SourceT> void operator() (SourceT & src, ValueType & v) const
  {
    v.m_pt = serial::LoadPoint(src, m_cp);
    v.m_featureId = ReadPrimitiveFromSource<uint32_t>(src);
    v.m_rank = ReadPrimitiveFromSource<uint8_t>(src);
  }

  template <class TSink> void Save(TSink & sink, ValueType const & v) const
  {
    serial::SavePoint(sink, v.m_pt, m_cp);
    WriteToSink(sink, v.m_featureId);
    WriteToSink(sink, v.m_rank);
  }
};

typedef EmptyValueReader EdgeValueReader;

typedef trie::Iterator<trie::ValueReader::ValueType, trie::EdgeValueReader::ValueType>
    DefaultIterator;

inline serial::CodingParams GetCodingParams(serial::CodingParams const & orig)
{
  return serial::CodingParams(search::POINT_CODING_BITS,
                              PointU2PointD(orig.GetBasePoint(), orig.GetCoordBits()));
}

}  // namespace trie
