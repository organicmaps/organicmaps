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
#include <limits>
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
  double constexpr kRankCoeff = 1.0;
  double constexpr kLangsCountCoeff = 1.0;
  double constexpr kAreaCoeff = 0.05;
  double constexpr kIsCapitalCoeff = 0.1;
  double constexpr kIsNodeCoeff = 0.15;
  double constexpr kIsAreaTooBigCoeff = -0.5;

  auto const normalizeRank = [](uint8_t rank) {
    return static_cast<double>(rank) / static_cast<double>(std::numeric_limits<uint8_t>::max());
  };

  auto const normalizeLangsCount = [](uint8_t langsCount) {
    return static_cast<double>(langsCount) /
           static_cast<double>(StringUtf8Multilang::kMaxSupportedLanguages);
  };

  auto const normalizeArea = [](double area) {
    // We need to compare areas to choose bigger feature from multipolygonal features.
    // |kMaxAreaM2| should be greater than cities exclaves (like airports or Zelenograd for Moscow).
    double const kMaxAreaM2 = 4e8;
    area = base::Clamp(area, 0.0, kMaxAreaM2);
    return area / kMaxAreaM2;
  };

  auto const isAreaTooBig = [](ftypes::LocalityType type, double area) {
    // 100*100 km. There are few such big cities in the world (Beijing, Tokyo). These cities are
    // well-maped and have node which we want to prefer because relation boundaries may include big
    // exclaves and/or have bad center: https://www.openstreetmap.org/relation/1543125.
    if (type == ftypes::LocalityType::City)
      return area > 1e10;

    // ~14*14 km
    if (type == ftypes::LocalityType::Town)
      return area > 2e8;

    // 10*10 km
    if (type == ftypes::LocalityType::Village)
      return area > 1e8;

    return false;
  };

  static_assert(kRankCoeff >= 0, "");
  static_assert(kLangsCountCoeff >= 0, "");
  static_assert(kAreaCoeff >= 0, "");
  static_assert(kIsCapitalCoeff >= 0, "");
  static_assert(kIsAreaTooBigCoeff <= 0, "");

  auto const getScore = [&](auto const place) {
    auto const rank = place.GetRank();
    auto const langsCount = place.GetMultilangName().CountLangs();
    auto const area = mercator::AreaOnEarth(place.GetLimitRect());

    auto const placeType = GetPlaceType(place);
    auto const isCapital = ftypes::IsCapitalChecker::Instance()(placeType);

    auto const type = place.GetMostGenericOsmId().GetType();
    auto const isNode = (type == base::GeoObjectId::Type::OsmNode) ||
                        (type == base::GeoObjectId::Type::ObsoleteOsmNode);

    auto const tooBig =
        isAreaTooBig(ftypes::IsLocalityChecker::Instance().GetType(placeType), area);

    return kRankCoeff * normalizeRank(rank) +
           kLangsCountCoeff * normalizeLangsCount(langsCount) +
           kAreaCoeff * normalizeArea(area) +
           kIsCapitalCoeff * (isCapital ? 1.0 : 0.0) +
           kIsNodeCoeff * (isNode ? 1.0 : 0.0) +
           kIsAreaTooBigCoeff * (tooBig ? 1.0 : 0.0);
  };

  return getScore(left) < getScore(right);
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
  if (fb.GetMultilangName().IsEmpty())
    return false;

  auto const & islandChecker = ftypes::IsIslandChecker::Instance();
  auto const & localityChecker = ftypes::IsLocalityChecker::Instance();
  return islandChecker(fb.GetTypes()) || localityChecker.GetType(GetPlaceType(fb)) != ftypes::LocalityType::None;
}

void FeaturePlace::Append(FeatureBuilder const & fb)
{
  if (m_fbs.empty() || IsWorsePlace(m_fbs[m_bestIndex], fb))
    m_bestIndex = m_fbs.size();

  m_fbs.emplace_back(fb);
  m_allFbsLimitRect.Add(fb.GetLimitRect());
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

m2::RectD const & FeaturePlace::GetLimitRect() const { return GetFb().GetLimitRect(); }

m2::RectD const & FeaturePlace::GetAllFbsLimitRect() const { return m_allFbsLimitRect; }

base::GeoObjectId FeaturePlace::GetMostGenericOsmId() const
{
  return GetFb().GetMostGenericOsmId();
}

uint8_t FeaturePlace::GetRank() const
{
  return GetFb().GetRank();
}

std::string_view FeaturePlace::GetName() const
{
  return GetFb().GetName();
}

StringUtf8Multilang const & FeaturePlace::GetMultilangName() const
{
  return GetFb().GetMultilangName();
}

bool FeaturePlace::IsPoint() const
{
  return GetFb().IsPoint();
}

m2::RectD GetLimitRect(FeaturePlace const & fp) { return fp.GetAllFbsLimitRect(); }

PlaceProcessor::PlaceProcessor(std::shared_ptr<OsmIdToBoundariesTable> boundariesTable)
  : m_boundariesTable(boundariesTable) {}

void PlaceProcessor::FillTable(FeaturePlaces::const_iterator start, FeaturePlaces::const_iterator end,
                               FeaturePlaces::const_iterator best) const
{
  CHECK(m_boundariesTable, ());
  base::GeoObjectId lastId;
  auto const & isCityTownOrVillage = ftypes::IsCityTownOrVillageChecker::Instance();
  for (auto outerIt = start; outerIt != end; ++outerIt)
  {
    auto const & fbs = outerIt->GetFbs();
    for (auto const & fb : fbs)
    {
      if (!(fb.IsArea() && isCityTownOrVillage(fb.GetTypes())))
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
        TransformToPoint(bestFb);

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

  std::string const name(fb.GetName());
  return name + "/" + std::to_string(type);
}

void PlaceProcessor::Add(FeatureBuilder const & fb)
{
  auto const type = GetPlaceType(fb);
  if (type == ftype::GetEmptyValue() || !NeedProcessPlace(fb))
    return;
  // Objects are grouped with the same name and type. This does not guarantee that all objects describe
  // the same place. The logic for the separation of different places of the same name is
  // implemented in the function ProcessPlaces().
  m_nameToPlaces[GetKey(fb)][fb.GetMostGenericOsmId()].Append(fb);
}
}  // namespace generator
