#pragma once

#include "coding/geometry_coding.hpp"
#include "coding/point_coding.hpp"

namespace search
{
static const uint8_t kCategoriesLang = 128;
static const uint8_t kPostcodesLang = 129;
static const uint8_t kPointCodingBits = 20;
}  // namespace search

namespace trie
{
inline serial::GeometryCodingParams GetGeometryCodingParams(
    serial::GeometryCodingParams const & orig)
{
  return serial::GeometryCodingParams(search::kPointCodingBits,
                                      PointUToPointD(orig.GetBasePoint(), orig.GetCoordBits()));
}
}  // namespace trie
