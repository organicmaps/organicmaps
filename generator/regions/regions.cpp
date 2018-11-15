#include "generator/regions/regions.hpp"

#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/generate_info.hpp"
#include "generator/regions/city.hpp"
#include "generator/regions/regions.hpp"
#include "generator/regions/node.hpp"
#include "generator/regions/regions_builder.hpp"
#include "generator/regions/regions_fixer.hpp"
#include "generator/regions/to_string_policy.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/transliteration.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/timer.hpp"

#include <algorithm>
#include <fstream>
#include <numeric>
#include <memory>
#include <queue>
#include <set>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "defines.hpp"

namespace generator
{
namespace regions
{
namespace
{
std::tuple<RegionsBuilder::Regions, PointCitiesMap>
ReadDatasetFromTmpMwm(std::string const & tmpMwmFilename, RegionInfo & collector)
{
  RegionsBuilder::Regions regions;
  PointCitiesMap pointCitiesMap;
  auto const toDo = [&](FeatureBuilder1 const & fb, uint64_t /* currPos */) {
    if (fb.IsArea() && fb.IsGeometryClosed())
    {
      auto const id = fb.GetMostGenericOsmId();
      auto region = Region(fb, collector.Get(id));
      regions.emplace_back(std::move(region));
    }
    else if (fb.IsPoint())
    {
      auto const id = fb.GetMostGenericOsmId();
      pointCitiesMap.emplace(id, City(fb, collector.Get(id)));
    }
  };

  feature::ForEachFromDatRawFormat(tmpMwmFilename, toDo);
  return std::make_tuple(std::move(regions), std::move(pointCitiesMap));
}

void FilterRegions(RegionsBuilder::Regions & regions)
{
  auto const pred = [](Region const & region) {
    auto const & label = region.GetLabel();
    auto const & name = region.GetName();
    return label.empty() || name.empty();
  };

  base::EraseIf(regions, pred);
}

RegionsBuilder::Regions ReadAndFixData(std::string const & tmpMwmFilename,
                                       RegionInfo & regionsInfoCollector)
{
  RegionsBuilder::Regions regions;
  PointCitiesMap pointCitiesMap;
  std::tie(regions, pointCitiesMap) = ReadDatasetFromTmpMwm(tmpMwmFilename, regionsInfoCollector);
  FixRegionsWithPlacePointApproximation(pointCitiesMap, regions);
  FilterRegions(regions);
  return regions;
}

void RepackTmpMwm(std::string const & srcFilename, std::string const & repackedFilename,
                  std::set<base::GeoObjectId> const & ids, RegionInfo const & regionInfo)
{
  feature::FeaturesCollector collector(repackedFilename);
  auto const toDo = [&collector, &ids, &regionInfo](FeatureBuilder1 & fb, uint64_t /* currPos */) {
    if (ids.count(fb.GetMostGenericOsmId()) == 0 ||
        (fb.IsPoint() && !FeatureCityPointToRegion(regionInfo, fb)))
    {
      return;
    }

    collector(fb);
  };

  feature::ForEachFromDatRawFormat(srcFilename, toDo);
}
}  // namespace

bool GenerateRegions(std::string const & pathInRegionsTmpMwm,
                     std::string const & pathInRegionsCollector,
                     std::string const & pathOutRegionsKv,
                     std::string const & pathOutRepackedRegionsTmpMwm, bool verbose)
{
  using namespace regions;

  LOG(LINFO, ("Start generating regions from ", pathInRegionsTmpMwm));
  auto timer = base::Timer();
  Transliteration::Instance().Init(GetPlatform().ResourcesDir());

  RegionInfo regionsInfoCollector(pathInRegionsCollector);
  RegionsBuilder::Regions regions = ReadAndFixData(pathInRegionsTmpMwm, regionsInfoCollector);
  auto jsonPolicy = std::make_unique<JsonPolicy>(verbose);
  auto kvBuilder = std::make_unique<RegionsBuilder>(std::move(regions), std::move(jsonPolicy));

  std::ofstream ofs(pathOutRegionsKv, std::ofstream::out);
  std::set<base::GeoObjectId> setIds;
  size_t countIds = 0;
  kvBuilder->ForEachNormalizedCountry([&](std::string const & name, Node::Ptr tree) {
    if (!tree)
      return;

    if (verbose)
      DebugPrintTree(tree);

    LOG(LINFO, ("Processing country", name));
    auto const idStringList = kvBuilder->ToIdStringList(tree);
    for (auto const & s : idStringList)
    {
      ofs << static_cast<int64_t>(s.first.GetEncodedId()) << " " << s.second << "\n";
      ++countIds;
      if (!setIds.insert(s.first).second)
        LOG(LWARNING, ("Id alredy exists:",  s.first));
    }
  });

  // todo(maksimandrianov1): Perhaps this is not the best solution. This is a hot fix. Perhaps it
  // is better to transfer this to index generation(function GenerateRegionsData),
  // or to combine index generation and key-value storage generation in
  // generator_tool(generator_tool.cpp).
  if (!pathOutRepackedRegionsTmpMwm.empty())
    RepackTmpMwm(pathInRegionsTmpMwm, pathOutRepackedRegionsTmpMwm, setIds, regionsInfoCollector);

  LOG(LINFO, ("Regions objects key-value for", kvBuilder->GetCountryNames().size(),
              "countries storage saved to",  pathOutRegionsKv));
  LOG(LINFO, ("Repacked regions temprory mwm saved to",  pathOutRepackedRegionsTmpMwm));
  LOG(LINFO, (countIds, "total ids.", setIds.size(), "unique ids."));
  LOG(LINFO, ("Finish generating regions.", timer.ElapsedSeconds(), "seconds."));
  return true;
}
}  // namespace regions
}  // namespace generator
