#include "generator/regions/regions.hpp"

#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/generate_info.hpp"
#include "generator/regions/city.hpp"
#include "generator/regions/node.hpp"
#include "generator/regions/regions_builder.hpp"
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
using PointCitiesMap = std::unordered_map<base::GeoObjectId, City>;

struct RegionsFixer
{
  RegionsFixer(RegionsBuilder::Regions & regions, PointCitiesMap const & pointCitiesMap)
    : m_regions(regions), m_pointCitiesMap(pointCitiesMap)
  {
    SplitRegionsByAdminCenter();
    CreateNameRegionMap();
  }

  RegionsBuilder::Regions & FixRegions()
  {
    SortRegionsByArea();
    std::vector<bool> unsuitable;
    unsuitable.resize(m_regionsWithAdminCenter.size());
    for (size_t i = 0; i < m_regionsWithAdminCenter.size(); ++i)
    {
      if (unsuitable[i])
        continue;

      auto & regionWithAdminCenter = m_regionsWithAdminCenter[i];
      if (regionWithAdminCenter.IsCountry())
        continue;

      auto const id = regionWithAdminCenter.GetAdminCenterId();
      if (!m_pointCitiesMap.count(id))
        continue;

      auto const & adminCenter = m_pointCitiesMap.at(id);
      auto const placeType = adminCenter.GetPlaceType();
      if (placeType == PlaceType::Town || placeType == PlaceType::City)
      {
        for (size_t j = i + 1; j + 1 < m_regionsWithAdminCenter.size(); ++j)
        {
          if (m_regionsWithAdminCenter[j].ContainsRect(regionWithAdminCenter))
            unsuitable[j] = true;
        }
      }

      if (RegionExistsAsCity(adminCenter))
        continue;

      regionWithAdminCenter.SetInfo(adminCenter);
    }

    std::move(std::begin(m_regionsWithAdminCenter), std::end(m_regionsWithAdminCenter),
              std::back_inserter(m_regions));
    m_regionsWithAdminCenter = {};
    return m_regions;
  }

private:
  bool RegionExistsAsCity(City const & city)
  {
    auto const range = m_nameRegionMap.equal_range(city.GetName());
    for (auto it = range.first; it != range.second; ++it)
    {
      Region const & r = it->second;
      if (city.GetRank() == r.GetRank() && r.Contains(city))
        return true;
    }

    return false;
  }

  void SplitRegionsByAdminCenter()
  {
    auto const pred = [](Region const & region) { return region.HasAdminCenter(); };
    std::copy_if(std::begin(m_regions), std::end(m_regions),
                 std::back_inserter(m_regionsWithAdminCenter), pred);
    base::EraseIf(m_regions, pred);
  }

  void CreateNameRegionMap()
  {
    for (auto const & region : m_regions)
    {
      auto const name = region.GetName();
      if (region.GetLabel() == "locality" && !name.empty())
        m_nameRegionMap.emplace(name, region);
    }
  }

  void SortRegionsByArea()
  {
    auto const cmp = [](Region const & l, Region const & r) { return l.GetArea() < r.GetArea(); };
    std::sort(std::begin(m_regions), std::end(m_regions), cmp);
  }

  RegionsBuilder::Regions & m_regions;
  PointCitiesMap const & m_pointCitiesMap;
  RegionsBuilder::Regions m_regionsWithAdminCenter;
  std::multimap<std::string, std::reference_wrapper<Region const>> m_nameRegionMap;
};

std::tuple<RegionsBuilder::Regions, PointCitiesMap>
ReadDatasetFromTmpMwm(std::string const & tmpMwmFilename, RegionInfo & collector)
{
  RegionsBuilder::Regions regions;
  PointCitiesMap pointCitiesMap;
  auto const toDo = [&regions, &pointCitiesMap, &collector](FeatureBuilder1 const & fb, uint64_t /* currPos */)
  {
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
  auto const pred = [](Region const & region)
  {
    auto const & label = region.GetLabel();
    auto const & name = region.GetName();
    return label.empty() || name.empty();
  };
  auto const it = std::remove_if(std::begin(regions), std::end(regions), pred);
  regions.erase(it, std::end(regions));
}

RegionsBuilder::Regions ReadData(std::string const & tmpMwmFilename,
                                 RegionInfo & regionsInfoCollector)
{
  RegionsBuilder::Regions regions;
  PointCitiesMap pointCitiesMap;
  std::tie(regions, pointCitiesMap) = ReadDatasetFromTmpMwm(tmpMwmFilename, regionsInfoCollector);
  RegionsFixer fixer(regions, pointCitiesMap);
  regions = fixer.FixRegions();
  FilterRegions(regions);
  return regions;
}

void RepackTmpMwm(std::string const & srcFilename, std::string const & repackedFilename,
                  std::set<base::GeoObjectId> const & ids)
{
  feature::FeaturesCollector collector(repackedFilename);
  auto const toDo = [&collector, &ids](FeatureBuilder1 const & fb, uint64_t /* currPos */)
  {
    if (ids.find(fb.GetMostGenericOsmId()) != std::end(ids))
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
  RegionsBuilder::Regions regions = ReadData(pathInRegionsTmpMwm, regionsInfoCollector);
  auto jsonPolicy = std::make_unique<JsonPolicy>(verbose);
  auto kvBuilder = std::make_unique<RegionsBuilder>(std::move(regions), std::move(jsonPolicy));
  auto const countryTrees = kvBuilder->GetCountryTrees();

  std::ofstream ofs(pathOutRegionsKv, std::ofstream::out);
  std::set<base::GeoObjectId> setIds;
  size_t countIds = 0;
  for (auto const & countryName : kvBuilder->GetCountryNames())
  {
    auto const tree = kvBuilder->GetNormalizedCountryTree(countryName);
    if (!tree)
      continue;

    if (verbose)
      DebugPrintTree(tree);

    auto const idStringList = kvBuilder->ToIdStringList(tree);
    for (auto const & s : idStringList)
    {
      ofs << static_cast<int64_t>(s.first.GetEncodedId()) << " " << s.second << "\n";
      ++countIds;
      if (!setIds.insert(s.first).second)
        LOG(LWARNING, ("Id alredy exists:",  s.first));
    }
  }

  // todo(maksimandrianov1): Perhaps this is not the best solution. This is a hot fix. Perhaps it
  // is better to transfer this to index generation(function GenerateRegionsData),
  // or to combine index generation and key-value storage generation in
  // generator_tool(generator_tool.cpp).
  if (!pathOutRepackedRegionsTmpMwm.empty())
    RepackTmpMwm(pathInRegionsTmpMwm, pathOutRepackedRegionsTmpMwm, setIds);

  LOG(LINFO, ("Regions objects key-value storage saved to",  pathOutRegionsKv));
  LOG(LINFO, ("Repacked regions temprory mwm saved to",  pathOutRepackedRegionsTmpMwm));
  LOG(LINFO, (countIds, "total ids.", setIds.size(), "unique ids."));
  LOG(LINFO, ("Finish generating regions.", timer.ElapsedSeconds(), "seconds."));
  return true;
}
}  // namespace regions
}  // namespace generator
