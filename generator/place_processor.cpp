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

namespace generator
{
using namespace feature;

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
bool IsWorsePlace(FeatureBuilder const & left, FeatureBuilder const & right)
{
  // Capital type should be always better.
  bool const lCapital = IsRealCapital(left);
  bool const rCapital = IsRealCapital(right);
  if (lCapital != rCapital)
    return rCapital;

  double constexpr kRankCoeff = 1.0;
  double constexpr kLangsCountCoeff = 1.0;
  double constexpr kAreaCoeff = 0.05;
  double constexpr kIsCapitalCoeff = 0.1;
  double constexpr kIsNodeCoeff = 0.15;
  double constexpr kIsAreaTooBigCoeff = -0.5;

  auto const normalizeRank = [](uint8_t rank)
  {
    return static_cast<double>(rank) / static_cast<double>(std::numeric_limits<uint8_t>::max());
  };

  auto const normalizeLangsCount = [](uint8_t langsCount)
  {
    return static_cast<double>(langsCount) /
           static_cast<double>(StringUtf8Multilang::kMaxSupportedLanguages);
  };

  auto const normalizeArea = [](double area)
  {
    // We need to compare areas to choose bigger feature from multipolygonal features.
    // |kMaxAreaM2| should be greater than cities exclaves (like airports or Zelenograd for Moscow).
    double const kMaxAreaM2 = 4e8;
    area = base::Clamp(area, 0.0, kMaxAreaM2);
    return area / kMaxAreaM2;
  };

  auto const isAreaTooBig = [](ftypes::LocalityType type, double area)
  {
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

  auto const getScore = [&](FeatureBuilder const & place)
  {
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

m2::RectD GetLimitRect(FeaturePlace const & fp) { return fp.GetAllFbsLimitRect(); }

std::vector<std::vector<FeaturePlace const *>> FindClusters(std::vector<FeaturePlace> const & places)
{
  auto const & localityChecker = ftypes::IsLocalityChecker::Instance();

  auto getRaduis = [&localityChecker](FeaturePlace const & fp)
  {
    return GetRadiusM(localityChecker.GetType(GetPlaceType(fp.GetFb())));
  };

  auto isSamePlace = [&localityChecker](FeaturePlace const & left, FeaturePlace const & right)
  {
    // See https://github.com/organicmaps/organicmaps/issues/2035 for more details.

    auto const & lFB = left.GetFb();
    auto const & rFB = right.GetFb();
    if (lFB.IsPoint() && rFB.IsPoint())
    {
      /// @todo Small hack here. Need to check enclosing/parent region.
      // Do not merge places with different ranks (population). See https://www.openstreetmap.org/#map=15/33.0145/-92.7249
      // Junction City in Arkansas and Louisiana.
      if (lFB.GetRank() > 0 && rFB.GetRank() > 0 && lFB.GetRank() != rFB.GetRank())
        return false;

      return localityChecker.GetType(GetPlaceType(lFB)) == localityChecker.GetType(GetPlaceType(rFB));
    }
    else
    {
      // Equal name is guaranteed here. Equal type is not necessary due to OSM relation vs node inconsistency in USA.
      return GetLimitRect(left).IsIntersect(GetLimitRect(right));
    }
  };

  return GetClusters(places, std::move(getRaduis), std::move(isSamePlace));
}

bool NeedProcessPlace(FeatureBuilder const & fb)
{
  if (fb.GetMultilangName().IsEmpty())
    return false;

  using namespace ftypes;
  auto const & islandChecker = IsIslandChecker::Instance();
  auto const & localityChecker = IsLocalityChecker::Instance();
  return islandChecker(fb.GetTypes()) || localityChecker.GetType(GetPlaceType(fb)) != LocalityType::None;
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

PlaceProcessor::PlaceProcessor(std::shared_ptr<OsmIdToBoundariesTable> boundariesTable)
  : m_boundariesTable(boundariesTable) {}

template <class IterT>
void PlaceProcessor::FillTable(IterT start, IterT end, IterT best) const
{
  base::GeoObjectId lastId;
  auto const & isCityTownOrVillage = ftypes::IsCityTownOrVillageChecker::Instance();
  for (auto it = start; it != end; ++it)
  {
    for (auto const & fb : (*it)->GetFbs())
    {
      if (!fb.IsArea() || !isCityTownOrVillage(fb.GetTypes()))
        continue;

      auto const id = fb.GetLastOsmId();
      m_boundariesTable->Append(id, indexer::CityBoundary(fb.GetOuterGeometry()));
      if (lastId != base::GeoObjectId())
        m_boundariesTable->Union(id, lastId);

      lastId = id;
    }
  }

  if (lastId != base::GeoObjectId())
    m_boundariesTable->Union(lastId, (*best)->GetFb().GetMostGenericOsmId());
}

std::vector<feature::FeatureBuilder> PlaceProcessor::ProcessPlaces(std::vector<IDsContainerT> * ids/* = nullptr*/)
{
  std::vector<feature::FeatureBuilder> finalPlaces;
  for (auto const & name2PlacesEntry : m_nameToPlaces)
  {
    std::vector<FeaturePlace> places;
    places.reserve(name2PlacesEntry.second.size());
    for (auto const & id2placeEntry : name2PlacesEntry.second)
      places.emplace_back(id2placeEntry.second);

    for (auto const & cluster : FindClusters(places))
    {
      auto best = std::max_element(cluster.begin(), cluster.end(), [](FeaturePlace const * l, FeaturePlace const * r)
      {
        return IsWorsePlace(l->GetFb(), r->GetFb());
      });
      auto bestFb = (*best)->GetFb();

      auto const & localityChecker = ftypes::IsLocalityChecker::Instance();
      if (bestFb.IsArea() && localityChecker.GetType(GetPlaceType(bestFb)) != ftypes::LocalityType::None)
        TransformToPoint(bestFb);

      if (ids)
      {
        ids->push_back({});
        auto & cont = ids->back();
        cont.reserve(cluster.size());
        base::Transform(cluster, std::back_inserter(cont), [](FeaturePlace const * place)
        {
          return place->GetFb().GetMostGenericOsmId();
        });
      }

      finalPlaces.emplace_back(std::move(bestFb));

      if (m_boundariesTable)
        FillTable(cluster.begin(), cluster.end(), best);
    }
  }

  return finalPlaces;
}

void PlaceProcessor::Add(FeatureBuilder const & fb)
{
  auto const type = GetPlaceType(fb);
  if (type == ftype::GetEmptyValue() || !NeedProcessPlace(fb))
    return;

  // Objects are grouped with the same name only. This does not guarantee that all objects describe the same place.
  // The logic for the separation of different places of the same name is implemented in the function ProcessPlaces().
  m_nameToPlaces[std::string(fb.GetName())][fb.GetMostGenericOsmId()].Append(fb);
}

}  // namespace generator
