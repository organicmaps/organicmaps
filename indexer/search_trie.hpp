#pragma once

#include "indexer/geometry_serialization.hpp"
#include "indexer/string_file_values.hpp"

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
using DefaultIterator = trie::Iterator<ValueList<FeatureIndexValue>>;

inline serial::CodingParams GetCodingParams(serial::CodingParams const & orig)
{
  return serial::CodingParams(search::kPointCodingBits,
                              PointU2PointD(orig.GetBasePoint(), orig.GetCoordBits()));
}

}  // namespace trie
