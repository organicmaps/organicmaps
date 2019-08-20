#include "generator/translator_region.hpp"

#include "generator/feature_maker.hpp"
#include "generator/filter_interface.hpp"
#include "generator/generate_info.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm_element_helpers.hpp"
#include "generator/regions/collector_region_info.hpp"

#include <algorithm>
#include <memory>
#include <set>
#include <string>

using namespace feature;

namespace generator
{
namespace
{
class FilterRegions : public FilterInterface
{
public:
  // FilterInterface overrides:
  std::shared_ptr<FilterInterface> Clone() const override
  {
    return std::make_shared<FilterRegions>();
  }

  bool IsAccepted(OsmElement const & element) override
  {
    for (auto const & t : element.Tags())
    {
      if (t.m_key == "place" && regions::EncodePlaceType(t.m_value) != regions::PlaceType::Unknown)
        return true;

      if (t.m_key == "boundary" && t.m_value == "administrative")
        return true;
    }

    return false;
  }

  bool IsAccepted(FeatureBuilder const & feature) override
  {
    return feature.GetParams().IsValid() && !feature.IsLine();
  }
};
}  // namespace

TranslatorRegion::TranslatorRegion(std::shared_ptr<FeatureProcessorInterface> const & processor,
                                   std::shared_ptr<cache::IntermediateData> const & cache,
                                   feature::GenerateInfo const & info)
  : Translator(processor, cache, std::make_shared<FeatureMakerSimple>(cache))

{
  SetFilter(std::make_shared<FilterRegions>());

  auto filename = info.GetTmpFileName(info.m_fileName, regions::CollectorRegionInfo::kDefaultExt);
  SetCollector(std::make_shared<regions::CollectorRegionInfo>(filename));
}

std::shared_ptr<TranslatorInterface>
TranslatorRegion::Clone() const
{
  return Translator::CloneBase<TranslatorRegion>();
}

void TranslatorRegion::Merge(TranslatorInterface const & other)
{
  other.MergeInto(*this);
}

void TranslatorRegion::MergeInto(TranslatorRegion & other) const
{
  MergeIntoBase(other);
}
}  // namespace generator
