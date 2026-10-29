#pragma once

#include "indexer/feature_decl.hpp"

#include "coding/reader.hpp"
#include "coding/varint.hpp"

#include <cstdint>
#include <set>
#include <utility>
#include <vector>

namespace df
{
uint8_t constexpr kMetaLinesSectionVersion = 3;

namespace detail
{
struct MetalineData
{
  std::vector<FeatureID> m_features;
  std::vector<double> m_dashPhaseOffsets;
  std::set<FeatureID> m_reversed;
};

template <typename Source>
std::vector<MetalineData> ReadMetalines(Source & src, MwmSet::MwmId const & mwmId)
{
  auto const version = ReadPrimitiveFromSource<uint8_t>(src);
  if (version < 1 || version > kMetaLinesSectionVersion)
    return {};

  std::vector<MetalineData> model;
  for (auto count = ReadVarUint<uint32_t>(src); count > 0; --count)
  {
    MetalineData data;
    for (auto i = ReadVarUint<uint32_t>(src); i > 0; --i)
    {
      int64_t const fid = ReadVarInt<int32_t>(src);
      FeatureID const featureId(mwmId, static_cast<uint32_t>(fid < 0 ? -fid : fid));
      data.m_features.push_back(featureId);
      // Experimental v2 stored float offsets; v3 uses doubles. Both are
      // interleaved with feature IDs, so even unused offsets must be consumed.
      if (version == 2)
        data.m_dashPhaseOffsets.push_back(ReadPrimitiveFromSource<float>(src));
      else if (version == 3)
        data.m_dashPhaseOffsets.push_back(ReadPrimitiveFromSource<double>(src));
      if (fid <= 0)
        data.m_reversed.insert(featureId);
    }
    if (!data.m_features.empty())
      model.push_back(std::move(data));
  }
  return model;
}
}  // namespace detail
}  // namespace df
