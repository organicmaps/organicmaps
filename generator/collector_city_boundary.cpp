#include "generator/collector_city_boundary.hpp"

#include "generator/feature_generator.hpp"
#include "generator/intermediate_data.hpp"

#include "indexer/ftypes_matcher.hpp"

#include <algorithm>
#include <iterator>

using namespace feature;

namespace generator
{
CityBoundaryCollector::CityBoundaryCollector(std::string const & filename)
  : CollectorInterface(filename) {}

std::shared_ptr<CollectorInterface>
CityBoundaryCollector::Clone(std::shared_ptr<cache::IntermediateDataReader> const &) const
{
  return std::make_shared<CityBoundaryCollector>(GetFilename());
}

void CityBoundaryCollector::CollectFeature(FeatureBuilder const & feature, OsmElement const &)
{
  if (feature.IsArea() && ftypes::IsCityTownOrVillage(feature.GetTypes()))
    m_boundaries.emplace_back(feature);
}

void CityBoundaryCollector::Save()
{
  FeatureBuilderWriter<serialization_policy::MaxAccuracy> collector(GetFilename());
  for (auto & boundary : m_boundaries)
  {
    if (boundary.PreSerialize())
      collector.Write(boundary);
  }
}

void CityBoundaryCollector::Merge(generator::CollectorInterface const * collector)
{
  CHECK(collector, ());

  collector->MergeInto(const_cast<CityBoundaryCollector *>(this));
}

void CityBoundaryCollector::MergeInto(CityBoundaryCollector * collector) const
{
  CHECK(collector, ());

  std::copy(std::begin(m_boundaries), std::end(m_boundaries),
            std::back_inserter(collector->m_boundaries));
}
}  // namespace generator
