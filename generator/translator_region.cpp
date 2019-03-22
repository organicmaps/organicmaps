#include "generator/translator_region.hpp"

#include "generator/feature_maker.hpp"
#include "generator/filter_interface.hpp"
#include "generator/generate_info.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm_element_helpers.hpp"
#include "generator/regions/collector_region_info.hpp"

#include <algorithm>
#include <set>
#include <string>

namespace generator
{
namespace
{
class FilterRegions : public FilterInterface
{
public:
  // FilterInterface overrides:
  bool IsAccepted(OsmElement const & element) override
  {
    for (auto const & t : element.Tags())
    {
      if (t.key == "place" && regions::EncodePlaceType(t.value) != regions::PlaceType::Unknown)
        return true;

      if (t.key == "boundary" && t.value == "administrative")
        return true;
    }

    return false;
  }

  bool IsAccepted(FeatureBuilder1 const & feature) override
  {
    return feature.GetParams().IsValid() && !feature.IsLine();
  }
};
}  // namespace

TranslatorRegion::TranslatorRegion(std::shared_ptr<EmitterInterface> emitter, cache::IntermediateDataReader & holder,
                                   feature::GenerateInfo const & info)
  : Translator(emitter, holder, std::make_shared<FeatureMakerSimple>(holder))

{
  AddFilter(std::make_shared<FilterRegions>());

  auto filename = info.GetTmpFileName(info.m_fileName, regions::CollectorRegionInfo::kDefaultExt);
  AddCollector(std::make_shared<regions::CollectorRegionInfo>(filename));
}
}  // namespace generator
