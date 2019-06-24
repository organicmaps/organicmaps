#include "generator/regions/regions.hpp"

#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/generate_info.hpp"
#include "generator/regions/node.hpp"
#include "generator/regions/place_point.hpp"
#include "generator/regions/regions.hpp"
#include "generator/regions/regions_builder.hpp"
#include "generator/regions/regions_fixer.hpp"
#include "generator/to_string_policy.hpp"

#include "platform/platform.hpp"

#include "coding/transliteration.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/timer.hpp"

#include <algorithm>
#include <fstream>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <set>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "defines.hpp"

using namespace feature;

namespace generator
{
namespace regions
{
namespace
{
class RegionsGenerator
{
public:
  RegionsGenerator(std::string const & pathInRegionsTmpMwm,
                   std::string const & pathInRegionsCollector, std::string const & pathOutRegionsKv,
                   std::string const & pathOutRepackedRegionsTmpMwm, bool verbose,
                   size_t threadsCount)
    : m_pathInRegionsTmpMwm{pathInRegionsTmpMwm}
    , m_pathOutRegionsKv{pathOutRegionsKv}
    , m_pathOutRepackedRegionsTmpMwm{pathOutRepackedRegionsTmpMwm}
    , m_verbose{verbose}
    , m_regionsInfoCollector{pathInRegionsCollector}
    , m_regionsKv{pathOutRegionsKv, std::ofstream::out}
  {
    LOG(LINFO, ("Start generating regions from", m_pathInRegionsTmpMwm));
    auto timer = base::Timer{};
    Transliteration::Instance().Init(GetPlatform().ResourcesDir());

    RegionsBuilder::Regions regions = ReadAndFixData();
    RegionsBuilder builder{std::move(regions), threadsCount};
    GenerateRegions(builder);

    LOG(LINFO, ("Finish generating regions.", timer.ElapsedSeconds(), "seconds."));
  }

private:
  void GenerateRegions(RegionsBuilder & builder)
  {
    builder.ForEachCountry([&](std::string const & name, Node::PtrList const & outers) {
      auto const & countryPlace = outers.front()->GetData();
      auto const & countryName = countryPlace.GetEnglishOrTransliteratedName();
      GenerateKv(countryName, outers);
    });

    LOG(LINFO, ("Regions objects key-value for", builder.GetCountryNames().size(),
                "countries storage saved to", m_pathOutRegionsKv));
    LOG(LINFO,
        (m_objectsRegions.size(), "total regions.", m_regionsCountries.size(), "total objects."));

    RepackTmpMwm();
  }

  void GenerateKv(std::string const & countryName, Node::PtrList const & outers)
  {
    LOG(LINFO, ("Generate country", countryName));

    auto country = std::make_shared<std::string>(countryName);
    size_t countryRegionsCount = 0;
    size_t countryObjectCount = 0;

    auto jsonPolicy = std::make_unique<JsonPolicy>(m_verbose);
    jsonPolicy->SetRealPrecision(JsonPolicy::kDefaultPrecision);

    for (auto const & tree : outers)
    {
      if (m_verbose)
        DebugPrintTree(tree);

      ForEachLevelPath(tree, [&](NodePath const & path) {
        auto const & node = path.back();
        auto const & region = node->GetData();
        auto const & objectId = region.GetId();
        auto const & regionCountryEmplace = m_regionsCountries.emplace(objectId, country);
        if (!regionCountryEmplace.second && regionCountryEmplace.first->second != country)
        {
          LOG(LWARNING, ("Failed to place", GetLabel(region.GetLevel()), "region", objectId, "(",
                         GetRegionNotation(region), ")", "into", *country,
                         ": region already exists in", *regionCountryEmplace.first->second));
          return;
        }

        m_objectsRegions.emplace(objectId, node);
        ++countryRegionsCount;

        if (regionCountryEmplace.second)
        {
          m_regionsKv << static_cast<int64_t>(objectId.GetEncodedId()) << " "
                      << jsonPolicy->ToString(path) << "\n";
          ++countryObjectCount;
        }
      });
    }

    LOG(LINFO, ("Country regions of", *country, "has built:", countryRegionsCount, "total regions.",
                countryObjectCount, "objects."));
  }

  std::tuple<RegionsBuilder::Regions, PlacePointsMap> ReadDatasetFromTmpMwm(
      std::string const & tmpMwmFilename, RegionInfo & collector)
  {
    RegionsBuilder::Regions regions;
    PlacePointsMap placePointsMap;
    auto const toDo = [&](FeatureBuilder const & fb, uint64_t /* currPos */) {
      if (fb.IsArea() && fb.IsGeometryClosed())
      {
        auto const id = fb.GetMostGenericOsmId();
        auto region = Region(fb, collector.Get(id));

        auto const & name = region.GetName();
        if (name.empty())
          return;

        regions.emplace_back(std::move(region));
      }
      else if (fb.IsPoint())
      {
        auto const id = fb.GetMostGenericOsmId();
        auto place = PlacePoint{fb, collector.Get(id)};

        auto const & name = place.GetName();
        auto const placeType = place.GetPlaceType();
        if (name.empty() || placeType == PlaceType::Unknown)
          return;

        placePointsMap.emplace(id, std::move(place));
      }
    };

    ForEachFromDatRawFormat(tmpMwmFilename, toDo);
    return std::make_tuple(std::move(regions), std::move(placePointsMap));
  }

  RegionsBuilder::Regions ReadAndFixData()
  {
    RegionsBuilder::Regions regions;
    PlacePointsMap placePointsMap;
    std::tie(regions, placePointsMap) =
        ReadDatasetFromTmpMwm(m_pathInRegionsTmpMwm, m_regionsInfoCollector);
    FixRegionsWithPlacePointApproximation(placePointsMap, regions);
    return regions;
  }

  void RepackTmpMwm()
  {
    feature::FeaturesCollector featuresCollector{m_pathOutRepackedRegionsTmpMwm};
    std::set<base::GeoObjectId> processedObjects;
    auto const toDo = [&](FeatureBuilder & fb, uint64_t /* currPos */) {
      auto const id = fb.GetMostGenericOsmId();
      auto objectRegions = m_objectsRegions.equal_range(id);
      if (objectRegions.first == objectRegions.second)
        return;
      if (!processedObjects.insert(id).second)
        return;

      for (auto item = objectRegions.first; item != objectRegions.second; ++item)
      {
        auto const & region = item->second->GetData();
        ResetGeometry(fb, region);
        fb.SetOsmId(region.GetId());
        fb.SetRank(0);
        featuresCollector.Collect(fb);
      }
    };

    LOG(LINFO, ("Start regions repacking from", m_pathInRegionsTmpMwm));
    feature::ForEachFromDatRawFormat(m_pathInRegionsTmpMwm, toDo);
    LOG(LINFO, ("Repacked regions temporary mwm saved to", m_pathOutRepackedRegionsTmpMwm));
  }

  void ResetGeometry(FeatureBuilder & fb, Region const & region)
  {
    fb.ResetGeometry();

    auto const & polygon = region.GetPolygon();
    auto outer = GetPointSeq(polygon->outer());
    fb.AddPolygon(outer);
    FeatureBuilder::Geometry holes;
    auto const & inners = polygon->inners();
    std::transform(std::begin(inners), std::end(inners), std::back_inserter(holes),
                   [this](auto && polygon) { return this->GetPointSeq(polygon); });
    fb.SetHoles(std::move(holes));
    fb.SetArea();

    CHECK(fb.IsArea(), ());
    CHECK(fb.IsGeometryClosed(), ());
  }

  template <typename Polygon>
  FeatureBuilder::PointSeq GetPointSeq(Polygon const & polygon)
  {
    FeatureBuilder::PointSeq seq;
    std::transform(std::begin(polygon), std::end(polygon), std::back_inserter(seq),
                   [](BoostPoint const & p) { return m2::PointD(p.get<0>(), p.get<1>()); });
    return seq;
  }

  std::string m_pathInRegionsTmpMwm;
  std::string m_pathOutRegionsKv;
  std::string m_pathOutRepackedRegionsTmpMwm;

  bool m_verbose{false};

  RegionInfo m_regionsInfoCollector;

  std::ofstream m_regionsKv;

  std::multimap<base::GeoObjectId, Node::Ptr> m_objectsRegions;
  std::map<base::GeoObjectId, std::shared_ptr<std::string>> m_regionsCountries;
};
}  // namespace

void GenerateRegions(std::string const & pathInRegionsTmpMwm,
                     std::string const & pathInRegionsCollector,
                     std::string const & pathOutRegionsKv,
                     std::string const & pathOutRepackedRegionsTmpMwm, bool verbose,
                     size_t threadsCount)
{
  RegionsGenerator(pathInRegionsTmpMwm, pathInRegionsCollector, pathOutRegionsKv,
                   pathOutRepackedRegionsTmpMwm, verbose, threadsCount);
}
}  // namespace regions
}  // namespace generator
