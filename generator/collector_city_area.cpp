#include "generator/collector_city_boundary.hpp"

#include "generator/feature_generator.hpp"
#include "generator/intermediate_data.hpp"

#include "indexer/ftypes_matcher.hpp"

#include <algorithm>
#include <iterator>

using namespace feature;

namespace generator
{
CityAreaCollector::CityAreaCollector(std::string const & filename)
  : CollectorInterface(filename) {}

std::shared_ptr<CollectorInterface>
CityAreaCollector::Clone(std::shared_ptr<cache::IntermediateDataReader> const &) const
{
  return std::make_shared<CityAreaCollector>(GetFilename());
}

void CityAreaCollector::CollectFeature(FeatureBuilder const & feature, OsmElement const &)
{
  if (feature.IsArea() && ftypes::IsCityTownOrVillage(feature.GetTypes()))
    m_boundaries.emplace_back(feature);
}

void CityAreaCollector::Save()
{
  FeatureBuilderWriter<serialization_policy::MaxAccuracy> collector(GetFilename());
  for (auto & boundary : m_boundaries)
  {
    if (boundary.PreSerialize())
      collector.Write(boundary);
  }
}

void CityAreaCollector::Merge(generator::CollectorInterface const & collector)
{
  collector.MergeInto(*this);
}

void CityAreaCollector::MergeInto(CityAreaCollector & collector) const
{
  std::copy(std::begin(m_boundaries), std::end(m_boundaries),
            std::back_inserter(collector.m_boundaries));
}
}  // namespace generator
