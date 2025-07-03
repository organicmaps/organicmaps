#include "generator/collector_boundary_postcode.hpp"

#include "generator/feature_builder.hpp"
#include "generator/intermediate_data.hpp"

#include "coding/read_write_utils.hpp"
#include "coding/string_utf8_multilang.hpp"

#include <algorithm>

namespace generator
{
BoundaryPostcodeCollector::BoundaryPostcodeCollector(std::string const & filename, IDRInterfacePtr const & cache)
  : CollectorInterface(filename)
  , m_cache(cache)
  , m_featureMakerSimple(cache)
{}

std::shared_ptr<CollectorInterface> BoundaryPostcodeCollector::Clone(IDRInterfacePtr const & cache) const
{
  return std::make_shared<BoundaryPostcodeCollector>(GetFilename(), cache ? cache : m_cache);
}

void BoundaryPostcodeCollector::Collect(OsmElement const & el)
{
  /// @todo Add postal_code for highways processing (along a street).

  // https://wiki.openstreetmap.org/wiki/Key:postal_code
  if (el.m_type != OsmElement::EntityType::Relation || el.GetTag("type") != "boundary")
    return;

  auto postcode = el.GetTag("postal_code");
  if (postcode.empty())
    postcode = el.GetTag("addr:postcode");

  // Filter dummy tags like here: https://www.openstreetmap.org/relation/7444
  if (postcode.empty() || postcode.find_first_of(";,") != std::string::npos)
    return;

  /// @todo We don't override CollectFeature instead, because of FeatureMakerSimple?
  auto osmElementCopy = el;
  feature::FeatureBuilder feature;
  m_featureMakerSimple.Add(osmElementCopy);

  while (m_featureMakerSimple.GetNextFeature(feature))
  {
    /// @todo Make move geometry?
    if (feature.IsGeometryClosed())
      m_data.emplace_back(postcode, feature.GetOuterGeometry());
  }
}

void BoundaryPostcodeCollector::Save()
{
  std::sort(m_data.begin(), m_data.end());

  FileWriter writer(GetFilename());
  for (auto const & p : m_data)
  {
    rw::WriteNonEmpty(writer, p.first);
    rw::WriteVectorOfPOD(writer, p.second);
  }
}

void BoundaryPostcodeCollector::MergeInto(BoundaryPostcodeCollector & collector) const
{
  collector.m_data.insert(collector.m_data.end(), m_data.begin(), m_data.end());
}
}  // namespace generator
