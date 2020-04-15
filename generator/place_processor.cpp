#include "generator/place_processor.hpp"

#include "generator/cluster_finder.hpp"
#include "generator/feature_maker_base.hpp"
#include "generator/type_helper.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <iterator>
#include <tuple>

using namespace feature;

namespace generator
{
uint32_t GetPlaceType(generator::FeaturePlace const & place)
{
  return GetPlaceType(place.GetFb());
}
}  // namespace generator

namespace
{
using namespace generator;

double GetRadiusM(ftypes::LocalityType const & type)
{
  switch (type)
  {
  case ftypes::LocalityType::Country: return 1000000.0;
  case ftypes::LocalityType::State: return 100000.0;
  case ftypes::LocalityType::City: return 30000.0;
  case ftypes::LocalityType::Town: return 20000.0;
  case ftypes::LocalityType::Village: return 5000.0;
  default: return 10000.0;
  }
}

// Returns true if left place is worse than right place.
template <typename T>
bool IsWorsePlace(T const & left, T const & right)
{
  auto const rankL = left.GetRank();
  auto const rankR = right.GetRank();
  auto const levelL = ftype::GetLevel(GetPlaceType(left));
  auto const levelR = ftype::GetLevel(GetPlaceType(right));
  auto const langCntL = left.GetMultilangName().CountLangs();
  auto const langCntR = right.GetMultilangName().CountLangs();
  auto const isPointL = left.IsPoint();
  auto const isPointR = right.IsPoint();
  auto const boxAreaL = left.GetLimitRect().Area();
  auto const boxAreaR = right.GetLimitRect().Area();
  return std::tie(rankL, levelL, langCntL, isPointL, boxAreaL) <
      std::tie(rankR, levelR, langCntR, isPointR, boxAreaR);
}

template <typename T>
bool IsTheSamePlace(T const & left, T const & right)
{
  if (left.GetName() != right.GetName())
    return false;

  auto const & localityChecker = ftypes::IsLocalityChecker::Instance();
  auto const localityL = localityChecker.GetType(GetPlaceType(left));
  auto const localityR = localityChecker.GetType(GetPlaceType(right));
  return localityL == localityR;
}

std::vector<std::vector<FeaturePlace>> FindClusters(std::vector<FeaturePlace> && places)
{
  auto const func = [](FeaturePlace const & fp) {
    auto const & localityChecker = ftypes::IsLocalityChecker::Instance();
    auto const locality = localityChecker.GetType(GetPlaceType(fp.GetFb()));
    return GetRadiusM(locality);
  };
  return GetClusters(std::move(places), func, IsTheSamePlace<FeaturePlace>);
}
}  // namespace

namespace generator
{
bool NeedProcessPlace(feature::FeatureBuilder const & fb)
{
  auto const & islandChecker = ftypes::IsIslandChecker::Instance();
  auto const & localityChecker = ftypes::IsLocalityChecker::Instance();
  return islandChecker(fb.GetTypes()) || localityChecker.GetType(GetPlaceType(fb)) != ftypes::LocalityType::None;
}

void FeaturePlace::Append(FeatureBuilder const & fb)
{
  if (m_fbs.empty() || IsWorsePlace(m_fbs[m_bestIndex], fb))
    m_bestIndex = m_fbs.size();

  m_fbs.emplace_back(fb);
  m_limitRect.Add(fb.GetLimitRect());
}

FeatureBuilder const & FeaturePlace::GetFb() const
{
  CHECK_LESS(m_bestIndex, m_fbs.size(), ());
  return m_fbs[m_bestIndex];
}

FeaturePlace::FeaturesBuilders const & FeaturePlace::GetFbs() const
{
  return m_fbs;
}

m2::RectD const & FeaturePlace::GetLimitRect() const
{
  return m_limitRect;
}

base::GeoObjectId FeaturePlace::GetMostGenericOsmId() const
{
  return GetFb().GetMostGenericOsmId();
}

uint8_t FeaturePlace::GetRank() const
{
  return GetFb().GetRank();
}

std::string FeaturePlace::GetName() const
{
  return GetFb().GetName();
}

m2::PointD FeaturePlace::GetKeyPoint() const
{
  return GetFb().GetKeyPoint();
}

StringUtf8Multilang const & FeaturePlace::GetMultilangName() const
{
  return GetFb().GetMultilangName();
}

bool FeaturePlace::IsPoint() const
{
  return GetFb().IsPoint();
}

m2::RectD GetLimitRect(FeaturePlace const & fp)
{
  return fp.GetLimitRect();
}

PlaceProcessor::PlaceProcessor(std::shared_ptr<OsmIdToBoundariesTable> boundariesTable)
  : m_boundariesTable(boundariesTable) {}

void PlaceProcessor::FillTable(FeaturePlaces::const_iterator start, FeaturePlaces::const_iterator end,
                               FeaturePlaces::const_iterator best) const
{
  CHECK(m_boundariesTable, ());
  base::GeoObjectId lastId;
  for (auto outerIt = start; outerIt != end; ++outerIt)
  {
    auto const & fbs = outerIt->GetFbs();
    for (auto const & fb : fbs)
    {
      if (!(fb.IsArea() && ftypes::IsCityTownOrVillage(fb.GetTypes())))
        continue;

      auto const id = fb.GetLastOsmId();
      m_boundariesTable->Append(id, indexer::CityBoundary(fb.GetOuterGeometry()));
      if (lastId != base::GeoObjectId())
        m_boundariesTable->Union(id, lastId);

      lastId = id;
    }
  }

  if (lastId != base::GeoObjectId())
    m_boundariesTable->Union(lastId, best->GetFb().GetMostGenericOsmId());
}

std::vector<PlaceProcessor::PlaceWithIds> PlaceProcessor::ProcessPlaces()
{
  std::vector<PlaceWithIds> finalPlaces;
  for (auto & nameToGeoObjectIdToFeaturePlaces : m_nameToPlaces)
  {
    std::vector<FeaturePlace> places;
    places.reserve(nameToGeoObjectIdToFeaturePlaces.second.size());
    for (auto const & geoObjectIdToFeaturePlaces : nameToGeoObjectIdToFeaturePlaces.second)
      places.emplace_back(geoObjectIdToFeaturePlaces.second);

    auto const clusters = FindClusters(std::move(places));
    for (auto const & cluster : clusters)
    {
      auto best = std::max_element(std::cbegin(cluster), std::cend(cluster), IsWorsePlace<FeaturePlace>);
      auto bestFb = best->GetFb();
      auto const & localityChecker = ftypes::IsLocalityChecker::Instance();
      if (bestFb.IsArea() && localityChecker.GetType(GetPlaceType(bestFb)) != ftypes::LocalityType::None)
      {
        LOG(LWARNING, (bestFb, "is transforming to point."));
        TransformToPoint(bestFb);
      }
      std::vector<base::GeoObjectId> ids;
      ids.reserve(cluster.size());
      base::Transform(cluster, std::back_inserter(ids), [](auto const & place) {
        return place.GetMostGenericOsmId();
      });
      finalPlaces.emplace_back(std::move(bestFb), std::move(ids));
      if (m_boundariesTable)
        FillTable(std::cbegin(cluster), std::cend(cluster), best);
    }
  }

  return finalPlaces;
}

// static
std::string PlaceProcessor::GetKey(FeatureBuilder const & fb)
{
  auto type = GetPlaceType(fb);
  ftype::TruncValue(type, 2);
  return fb.GetName() + "/" + std::to_string(type);
}

void PlaceProcessor::Add(FeatureBuilder const & fb)
{
  auto const type = GetPlaceType(fb);
  if (type == ftype::GetEmptyValue() || !NeedProcessPlace(fb))
    return;
  // Objects are grouped with the same name and type. This does not guarantee that all objects describe
  // the same place. The logic for the separation of different places of the same name is
  // implemented in the function GetPlaces().
  m_nameToPlaces[GetKey(fb)][fb.GetMostGenericOsmId()].Append(fb);
}
}  // namespace generator
