#include "indexer/ftypes_matcher.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <unordered_map>

namespace ftypes
{
namespace
{
class HighwayClasses
{
  std::unordered_map<uint32_t, HighwayClass> m_map;

public:
  HighwayClasses()
  {
    auto const & c = classif();
    m_map[c.GetTypeByPath({"route", "ferry"})] = HighwayClass::Transported;
    m_map[c.GetTypeByPath({"route", "shuttle_train"})] = HighwayClass::Transported;

    for (char const * v : {"motorway", "motorway_link", "trunk", "trunk_link"})
      m_map[c.GetTypeByPath({"highway", v})] = HighwayClass::Trunk;

    m_map[c.GetTypeByPath({"highway", "primary"})] = HighwayClass::Primary;
    m_map[c.GetTypeByPath({"highway", "primary_link"})] = HighwayClass::Primary;

    m_map[c.GetTypeByPath({"highway", "secondary"})] = HighwayClass::Secondary;
    m_map[c.GetTypeByPath({"highway", "secondary_link"})] = HighwayClass::Secondary;

    m_map[c.GetTypeByPath({"highway", "tertiary"})] = HighwayClass::Tertiary;
    m_map[c.GetTypeByPath({"highway", "tertiary_link"})] = HighwayClass::Tertiary;

    for (char const * v : {"unclassified", "residential", "living_street", "road"})
      m_map[c.GetTypeByPath({"highway", v})] = HighwayClass::LivingStreet;

    for (char const * v : {"service", "track", "busway"})
      m_map[c.GetTypeByPath({"highway", v})] = HighwayClass::Service;
    m_map[c.GetTypeByPath({"man_made", "pier"})] = HighwayClass::Service;

    // 3-level types.
    m_map[c.GetTypeByPath({"highway", "service", "driveway"})] = HighwayClass::ServiceMinor;
    m_map[c.GetTypeByPath({"highway", "service", "parking_aisle"})] = HighwayClass::ServiceMinor;

    for (char const * v :
         {"pedestrian", "platform", "footway", "bridleway", "ladder", "steps", "cycleway", "path", "construction"})
      m_map[c.GetTypeByPath({"highway", v})] = HighwayClass::Pedestrian;
  }

  HighwayClass Get(uint32_t t) const
  {
    auto const it = m_map.find(t);
    if (it == m_map.cend())
      return HighwayClass::Undefined;
    return it->second;
  }
};

char const * HighwayClassToString(HighwayClass const cls)
{
  switch (cls)
  {
  case HighwayClass::Undefined: return "Undefined";
  case HighwayClass::Transported: return "Transported";
  case HighwayClass::Trunk: return "Trunk";
  case HighwayClass::Primary: return "Primary";
  case HighwayClass::Secondary: return "Secondary";
  case HighwayClass::Tertiary: return "Tertiary";
  case HighwayClass::LivingStreet: return "LivingStreet";
  case HighwayClass::Service: return "Service";
  case HighwayClass::ServiceMinor: return "ServiceMinor";
  case HighwayClass::Pedestrian: return "Pedestrian";
  case HighwayClass::Count: return "Count";
  }
  ASSERT(false, ());
  return "";
}
}  // namespace

std::string DebugPrint(HighwayClass const cls)
{
  return std::string{"[ "} + HighwayClassToString(cls) + " ]";
}

std::string DebugPrint(LocalityType const localityType)
{
  switch (localityType)
  {
  case LocalityType::None: return "None";
  case LocalityType::Country: return "Country";
  case LocalityType::State: return "State";
  case LocalityType::City: return "City";
  case LocalityType::Town: return "Town";
  case LocalityType::Village: return "Village";
  case LocalityType::Count: return "Count";
  }
  UNREACHABLE();
}

HighwayClass GetHighwayClass(feature::TypesHolder const & types)
{
  static HighwayClasses highwayClasses;

  for (auto t : types)
  {
    for (uint8_t level : {3, 2})
    {
      ftype::TruncValue(t, level);
      HighwayClass const hc = highwayClasses.Get(t);
      if (hc != HighwayClass::Undefined)
        return hc;
    }
  }

  return HighwayClass::Undefined;
}

/////////////////////////////////////////////////////////////////////////////////////////
// BaseChecker implementation

void BaseChecker::PostInitialize()
{
  ASSERT(!m_types.empty(), ());

  // I don't think that binary_search is better for _almost_ always small vectors here.
  // std::sort(m_types.begin(), m_types.end());
}

uint32_t BaseChecker::PrepareToMatch(uint32_t type, uint8_t level)
{
  return ftype::Trunc(type, level);
}

bool BaseChecker::IsMatched(uint32_t type) const
{
  // return std::binary_search(m_types.begin(), m_types.end(), PrepareToMatch(type, m_level));
  return base::IsExist(m_types, PrepareToMatch(type, m_level));
}

bool BaseChecker::operator()(FeatureType & ft) const
{
  return this->operator()(feature::TypesHolder(ft));
}

/////////////////////////////////////////////////////////////////////////////////////////
// BaseCheckerEx implementation

BaseCheckerEx::BaseCheckerEx(std::initializer_list<base::StringIL> const & lst)
{
  Classificator const & c = classif();
  m_types.reserve(lst.size());
  for (auto const & e : lst)
  {
    uint32_t const t = c.GetTypeByPath(e);
    m_types.emplace_back(t, e.size());
    ASSERT_EQUAL(ftype::GetLevel(t), e.size(), ());
  }
}

bool BaseCheckerEx::operator()(FeatureType & ft) const
{
  return this->operator()(feature::TypesHolder(ft));
}

/////////////////////////////////////////////////////////////////////////////////////////
// Checkers implementation

IsPeakChecker::IsPeakChecker() : BaseCheckerEx({{"natural", "peak"}}) {}
IsATMChecker::IsATMChecker() : BaseCheckerEx({{"amenity", "atm"}}) {}
IsSpeedCamChecker::IsSpeedCamChecker() : BaseCheckerEx({{"highway", "speed_camera"}}) {}
IsPostBoxChecker::IsPostBoxChecker() : BaseCheckerEx({{"amenity", "post_box"}}) {}

IsPostPoiChecker::IsPostPoiChecker()
{
  Classificator const & c = classif();
  for (char const * val : {"post_office", "post_box", "parcel_locker"})
    m_types.push_back(c.GetTypeByPath({"amenity", val}));
}

IsOperatorOthersPoiChecker::IsOperatorOthersPoiChecker()
{
  Classificator const & c = classif();
  for (char const * val :
       {"bicycle_rental", "bureau_de_change", "car_sharing", "car_rental", "fuel", "charging_station", "parking",
        "motorcycle_parking", "bicycle_parking", "payment_terminal", "university", "vending_machine"})
    m_types.push_back(c.GetTypeByPath({"amenity", val}));
}

IsRecyclingCentreChecker::IsRecyclingCentreChecker() : BaseCheckerEx({{"amenity", "recycling", "centre"}}) {}

IsRecyclingContainerChecker::IsRecyclingContainerChecker() : BaseChecker(3 /* level */)
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({"amenity", "recycling", "container"}));
  // Treat default type also as a container, see https://taginfo.openstreetmap.org/keys/recycling_type#values
  m_types.push_back(c.GetTypeByPath({"amenity", "recycling"}));
}

IsRailwayStationChecker::IsRailwayStationChecker()
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({"railway", "station"}));
  m_types.push_back(c.GetTypeByPath({"building", "train_station"}));
}

IsSubwayStationChecker::IsSubwayStationChecker() : BaseCheckerEx({{"railway", "station", "subway"}}) {}
IsAirportChecker::IsAirportChecker() : BaseCheckerEx({{"aeroway", "aerodrome"}}) {}
IsSquareChecker::IsSquareChecker() : BaseCheckerEx({{"place", "square"}}) {}

IsSuburbChecker::IsSuburbChecker()
{
  Classificator const & c = classif();
  base::StringIL const types[] = {
      {"landuse", "residential"}, {"place", "neighbourhood"}, {"place", "quarter"}, {"place", "suburb"}};
  for (auto const & e : types)
    m_types.push_back(c.GetTypeByPath(e));

  // `types` order should match next indices.
  static_assert(static_cast<size_t>(SuburbType::Residential) == 0);
  static_assert(static_cast<size_t>(SuburbType::Neighbourhood) == 1);
  static_assert(static_cast<size_t>(SuburbType::Quarter) == 2);
  static_assert(static_cast<size_t>(SuburbType::Suburb) == 3);
}

SuburbType IsSuburbChecker::GetType(uint32_t t) const
{
  ftype::TruncValue(t, 2);
  for (size_t i = 0; i < m_types.size(); ++i)
    if (m_types[i] == t)
      return static_cast<SuburbType>(i);
  return SuburbType::Count;
}

SuburbType IsSuburbChecker::GetType(feature::TypesHolder const & types) const
{
  auto smallestType = SuburbType::Count;
  for (uint32_t t : types)
  {
    auto const type = GetType(t);
    if (type < smallestType)
      smallestType = type;
  }
  return smallestType;
}

SuburbType IsSuburbChecker::GetType(FeatureType & f) const
{
  feature::TypesHolder types(f);
  return GetType(types);
}

IsWayChecker::IsWayChecker()
{
  Classificator const & c = classif();
  std::pair<char const *, SearchRank> const types[] = {
      // type           rank
      {"cycleway", Cycleway},     {"footway", Pedestrian},      {"living_street", Residential},
      {"motorway", Motorway},     {"motorway_link", Motorway},  {"path", Outdoor},
      {"pedestrian", Pedestrian}, {"platform", Pedestrian},     {"primary", Regular},
      {"primary_link", Regular},  {"residential", Residential}, {"road", Minors},
      {"secondary", Regular},     {"secondary_link", Regular},  {"service", Minors},
      {"ladder", Pedestrian},     {"steps", Pedestrian},        {"tertiary", Regular},
      {"tertiary_link", Regular}, {"track", Outdoor},           {"trunk", Motorway},
      {"trunk_link", Motorway},   {"unclassified", Minors},
  };

  m_ranks.Reserve(std::size(types));
  for (auto const & e : types)
  {
    uint32_t const type = c.GetTypeByPath({"highway", e.first});
    m_types.push_back(type);
    m_ranks.Insert(type, e.second);
  }
  m_ranks.FinishBuilding();
}

IsWayChecker::SearchRank IsWayChecker::GetSearchRank(uint32_t type) const
{
  ftype::TruncValue(type, 2);
  if (auto const * res = m_ranks.Find(type))
    return *res;
  return Default;
}

bool IsStreetOrSquareChecker::operator()(FeatureType & ft) const
{
  auto const geomType = ft.GetGeomType();
  // Highway should be line or area.
  bool const isLineOrArea = (geomType == feature::GeomType::Line || geomType == feature::GeomType::Area);
  // Square also maybe a point (besides line or area).
  feature::TypesHolder types(ft);
  return ((isLineOrArea && m_street(types)) || m_square(types));
}

IsWayChecker::SearchRank IsStreetOrSquareChecker::GetSearchRank(uint32_t type) const
{
  auto rank = m_street.GetSearchRank(type);
  if (rank == IsWayChecker::Default && m_square(type))
    rank = IsWayChecker::Square;
  return rank;
}

// Used to determine for which features to display address in PP and in search results.
// If such a feature has a housenumber and a name then its enriched with a postcode (at the generation stage).
IsAddressObjectChecker::AddressOneLevel::AddressOneLevel() : BaseChecker(1 /* level */)
{
  m_types = OneLevelPOIChecker().GetTypes();

  Classificator const & c = classif();
  for (auto const * p : {"addr:interpolation", "building", "entrance", "landuse"})
    m_types.push_back(c.GetTypeByPath({p}));
}

IsAddressObjectChecker::AddressTwoLevel::AddressTwoLevel() : BaseChecker(2 /* level */)
{
  // Introduced 2-level checker to avoid types like barrier=fence.
  base::StringIL const arr[] = {
      {"aeroway", "aerodrome"}, {"barrier", "gate"},   {"barrier", "wicket_gate"},
      {"man_made", "mast"},     {"man_made", "works"}, {"man_made", "tower"},
      {"power", "generator"},   {"power", "plant"},    {"power", "substation"},
  };

  Classificator const & c = classif();
  for (auto const & path : arr)
    m_types.push_back(c.GetTypeByPath(path));
}

// Used to insert exact address (street and house number) instead of
// an empty name in search results (see ranker.cpp)
IsAddressChecker::IsAddressChecker() : BaseChecker(1 /* level */)
{
  Classificator const & c = classif();
  for (auto const * p : {"addr:interpolation", "building", "entrance"})
    m_types.push_back(c.GetTypeByPath({p}));
}

IsVillageChecker::IsVillageChecker()
{
  // TODO (@y, @m, @vng): this list must be up-to-date with
  // data/categories.txt, so, it's worth it to generate or parse it
  // from that file.
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({"place", "village"}));
  m_types.push_back(c.GetTypeByPath({"place", "hamlet"}));
}

IsOneWayChecker::IsOneWayChecker() : BaseCheckerEx({{"hwtag", "oneway"}}) {}

IsRoundAboutChecker::IsRoundAboutChecker()
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({"junction", "circular"}));
  m_types.push_back(c.GetTypeByPath({"junction", "roundabout"}));
}

IsLinkChecker::IsLinkChecker()
{
  Classificator const & c = classif();
  base::StringIL const types[] = {{"highway", "motorway_link"},
                                  {"highway", "trunk_link"},
                                  {"highway", "primary_link"},
                                  {"highway", "secondary_link"},
                                  {"highway", "tertiary_link"}};

  for (auto const & e : types)
    m_types.push_back(c.GetTypeByPath(e));
}

IsBuildingChecker::IsBuildingChecker() : BaseCheckerEx({{"building"}}) {}
IsBuildingPartChecker::IsBuildingPartChecker() : BaseCheckerEx({{"building:part"}}) {}
IsBuildingHasPartsChecker::IsBuildingHasPartsChecker() : BaseCheckerEx({{"building", "has_parts"}}) {}
IsIsolineChecker::IsIsolineChecker() : BaseCheckerEx({{"isoline"}}) {}
IsPisteChecker::IsPisteChecker() : BaseCheckerEx({{"piste:type"}}) {}
IsMwmBorderChecker::IsMwmBorderChecker() : BaseCheckerEx({{"organicapp", "mwm_border"}}) {}

// Used in IsPoiChecker and in IsAddressObjectChecker.
OneLevelPOIChecker::OneLevelPOIChecker() : ftypes::BaseChecker(1 /* level */)
{
  Classificator const & c = classif();

  for (auto const * path : {"amenity", "craft", "emergency", "healthcare", "historic", "leisure", "mountain_pass",
                            "office", "railway", "shop", "sport", "tourism"})
    m_types.push_back(c.GetTypeByPath({path}));
}

// Used in IsPoiChecker and also in TypesSkipper to keep types in the search index.
TwoLevelPOIChecker::TwoLevelPOIChecker() : ftypes::BaseChecker(2 /* level */)
{
  Classificator const & c = classif();
  base::StringIL arr[] = {{"aeroway", "terminal"},       {"aeroway", "gate"},         {"building", "guardhouse"},
                          {"building", "train_station"}, {"highway", "bus_stop"},     {"highway", "elevator"},
                          {"highway", "ford"},           {"highway", "raceway"},      {"highway", "rest_area"},
                          {"highway", "services"},       {"highway", "speed_camera"}, {"man_made", "cross"},
                          {"man_made", "lighthouse"},    {"man_made", "water_tap"},   {"man_made", "water_well"},
                          {"man_made", "windmill"},      {"natural", "beach"},        {"natural", "cave_entrance"},
                          {"natural", "geyser"},         {"natural", "hot_spring"},   {"natural", "peak"},
                          {"natural", "saddle"},         {"natural", "spring"},       {"natural", "volcano"},
                          {"waterway", "waterfall"}};

  for (auto const & path : arr)
    m_types.push_back(c.GetTypeByPath(path));
}

IsAmenityChecker::IsAmenityChecker() : BaseCheckerEx({{"amenity"}}) {}

AttractionsChecker::AttractionsChecker() : BaseChecker(2 /* level */)
{
  base::StringIL const primaryAttractionTypes[] = {
      {"amenity", "arts_centre"},
      {"amenity", "grave_yard"},
      {"amenity", "fountain"},
      {"amenity", "place_of_worship"},
      {"amenity", "theatre"},
      {"amenity", "townhall"},
      {"amenity", "university"},
      {"boundary", "national_park"},
      {"building", "train_station"},
      {"highway", "pedestrian"},
      {"historic", "archaeological_site"},
      {"historic", "boundary_stone"},
      {"historic", "castle"},
      {"historic", "city_gate"},
      {"historic", "citywalls"},
      {"historic", "fort"},
      {"historic", "gallows"},
      {"historic", "memorial"},
      {"historic", "monument"},
      {"historic", "locomotive"},
      {"historic", "tank"},
      {"historic", "aircraft"},
      {"historic", "pillory"},
      {"historic", "ruins"},
      {"historic", "ship"},
      {"historic", "tomb"},
      {"historic", "wayside_cross"},
      {"historic", "wayside_shrine"},
      {"landuse", "cemetery"},
      {"leisure", "beach_resort"},
      {"leisure", "garden"},
      {"leisure", "marina"},
      {"leisure", "nature_reserve"},
      {"leisure", "park"},
      {"leisure", "water_park"},
      {"man_made", "lighthouse"},
      {"man_made", "windmill"},
      {"natural", "beach"},
      {"natural", "cave_entrance"},
      {"natural", "geyser"},
      {"natural", "glacier"},
      {"natural", "hot_spring"},
      {"natural", "peak"},
      {"natural", "volcano"},
      {"place", "square"},
      {"tourism", "aquarium"},
      {"tourism", "artwork"},
      {"tourism", "museum"},
      {"tourism", "gallery"},
      {"tourism", "zoo"},
      {"tourism", "theme_park"},
      {"waterway", "waterfall"},
  };

  Classificator const & c = classif();

  for (auto const & e : primaryAttractionTypes)
    m_types.push_back(c.GetTypeByPath(e));
  std::sort(m_types.begin(), m_types.end());
  m_additionalTypesStart = m_types.size();

  // Additional types are worse in "hierarchy" priority.
  base::StringIL const additionalAttractionTypes[] = {
      {"tourism", "viewpoint"},
      {"tourism", "attraction"},
  };

  for (auto const & e : additionalAttractionTypes)
    m_types.push_back(c.GetTypeByPath(e));
  std::sort(m_types.begin() + m_additionalTypesStart, m_types.end());
}

uint32_t AttractionsChecker::GetBestType(FeatureParams::Types const & types) const
{
  auto additionalType = ftype::GetEmptyValue();
  auto const itAdditional = m_types.begin() + m_additionalTypesStart;

  for (auto type : types)
  {
    type = PrepareToMatch(type, m_level);
    if (std::binary_search(m_types.begin(), itAdditional, type))
      return type;

    if (std::binary_search(itAdditional, m_types.end(), type))
      additionalType = type;
  }

  return additionalType;
}

IsPlaceChecker::IsPlaceChecker() : BaseCheckerEx({{"place"}}) {}

IsBridgeOrTunnelChecker::IsBridgeOrTunnelChecker() : BaseChecker(3 /* level */) {}

bool IsBridgeOrTunnelChecker::IsMatched(uint32_t type) const
{
  if (ftype::GetLevel(type) != 3)
    return false;

  ClassifObject const * p = classif().GetRoot()->GetObject(ftype::GetValue(type, 0));
  if (p->GetName() != "highway")
    return false;

  p = p->GetObject(ftype::GetValue(type, 1));
  p = p->GetObject(ftype::GetValue(type, 2));
  return (p->GetName() == "bridge" || p->GetName() == "tunnel");
}

IsHotelChecker::IsHotelChecker()
{
  base::StringIL const types[] = {
      {"tourism", "alpine_hut"},   {"tourism", "apartment"},   {"tourism", "camp_site"},
      {"tourism", "caravan_site"},  /// @todo Sure here?
      {"tourism", "chalet"},       {"tourism", "guest_house"}, {"tourism", "hostel"},
      {"tourism", "hotel"},        {"tourism", "motel"},       {"tourism", "wilderness_hut"},
      {"leisure", "resort"},
  };

  Classificator const & c = classif();
  for (auto const & e : types)
    m_types.push_back(c.GetTypeByPath(e));
}

IsCampPitchChecker::IsCampPitchChecker() : BaseCheckerEx({{"tourism", "camp_pitch"}}) {}

IsIslandChecker::IsIslandChecker()
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({"place", "island"}));
  m_types.push_back(c.GetTypeByPath({"place", "islet"}));
}

IsLandChecker::IsLandChecker() : BaseCheckerEx({{"natural", "land"}}) {}
IsCoastlineChecker::IsCoastlineChecker() : BaseCheckerEx({{"natural", "coastline"}}) {}
IsWifiChecker::IsWifiChecker() : BaseCheckerEx({{"internet_access", "wlan"}}) {}

IsEatChecker::IsEatChecker()
{
  // The order should be the same as in "enum class Type" declaration.
  /// @todo amenity=ice_cream if we already have biergarten :)
  base::StringIL const types[] = {
      {"amenity", "cafe"}, {"amenity", "fast_food"},  {"amenity", "restaurant"}, {"amenity", "bar"},
      {"amenity", "pub"},  {"amenity", "biergarten"}, {"amenity", "food_court"},
  };

  Classificator const & c = classif();
  //  size_t i = 0;
  for (auto const & e : types)
  {
    auto const type = c.GetTypeByPath(e);
    m_types.push_back(type);
    //    m_eat2clType[i++] = type;
  }
}

// IsEatChecker::Type IsEatChecker::GetType(uint32_t t) const
//{
//   for (size_t i = 0; i < m_eat2clType.size(); ++i)
//   {
//     if (m_eat2clType[i] == t)
//       return static_cast<Type>(i);
//   }
//   return IsEatChecker::Type::Count;
// }

IsCuisineChecker::IsCuisineChecker() : BaseCheckerEx({{"cuisine"}}) {}
IsRecyclingTypeChecker::IsRecyclingTypeChecker() : BaseCheckerEx({{"recycling"}}) {}
IsFeeTypeChecker::IsFeeTypeChecker() : BaseCheckerEx({{"fee"}}) {}

IsToiletsChecker::IsToiletsChecker() : BaseChecker(2 /* level */)
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({"amenity", "toilets"}));
  m_types.push_back(c.GetTypeByPath({"toilets", "yes"}));
}

IsCapitalChecker::IsCapitalChecker() : BaseCheckerEx({{"place", "city", "capital"}}) {}
IsParkingChecker::IsParkingChecker() : BaseCheckerEx({{"amenity", "parking"}}) {}
IsCarChargingChecker::IsCarChargingChecker() : BaseCheckerEx({{"amenity", "charging_station", "motorcar"}}) {}

IsBicycleParkingChecker::IsBicycleParkingChecker()
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({"amenity", "bicycle_parking"}));
  m_types.push_back(c.GetTypeByPath({"amenity", "bicycle_rental"}));
}

IsBicycleChargingChecker::IsBicycleChargingChecker() : BaseCheckerEx({{"amenity", "charging_station", "bicycle"}}) {}
IsMotorcycleParkingChecker::IsMotorcycleParkingChecker() : BaseCheckerEx({{"amenity", "motorcycle_parking"}}) {}

IsPublicTransportStopChecker::IsPublicTransportStopChecker()
{
  Classificator const & c = classif();
  /// @todo Add bus station into _major_ class (like IsRailwayStationChecker)?
  m_types.push_back(c.GetTypeByPath({"aerialway", "station"}));
  m_types.push_back(c.GetTypeByPath({"amenity", "bus_station"}));
  m_types.push_back(c.GetTypeByPath({"amenity", "ferry_terminal"}));
  m_types.push_back(c.GetTypeByPath({"highway", "bus_stop"}));
  m_types.push_back(c.GetTypeByPath({"railway", "halt"}));
  m_types.push_back(c.GetTypeByPath({"railway", "tram_stop"}));
}

IsTaxiChecker::IsTaxiChecker() : BaseCheckerEx({{"amenity", "taxi"}}) {}
IsMotorwayJunctionChecker::IsMotorwayJunctionChecker() : BaseCheckerEx({{"highway", "motorway_junction"}}) {}

IsWayWithDurationChecker::IsWayWithDurationChecker()
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({"route", "ferry"}));
  m_types.push_back(c.GetTypeByPath({"route", "shuttle_train"}));
}

LocalityType LocalityFromString(std::string_view place)
{
  if (place == "village" || place == "hamlet")
    return LocalityType::Village;
  if (place == "town")
    return LocalityType::Town;
  if (place == "city")
    return LocalityType::City;
  if (place == "state")
    return LocalityType::State;
  if (place == "country")
    return LocalityType::Country;

  return LocalityType::None;
}

IsLocalityChecker::IsLocalityChecker()
{
  Classificator const & c = classif();

  // Note! The order should be equal with constants in Type enum (add other villages to the end).
  base::StringIL const types[] = {{"place", "country"}, {"place", "state"},   {"place", "city"},
                                  {"place", "town"},    {"place", "village"}, {"place", "hamlet"}};

  for (auto const & e : types)
    m_types.push_back(c.GetTypeByPath(e));
}

LocalityType IsLocalityChecker::GetType(uint32_t t) const
{
  ftype::TruncValue(t, 2);

  auto j = static_cast<size_t>(LocalityType::Country);
  for (; j < static_cast<size_t>(LocalityType::Count); ++j)
    if (t == m_types[j])
      return static_cast<LocalityType>(j);

  for (; j < m_types.size(); ++j)
    if (t == m_types[j])
      return LocalityType::Village;

  return LocalityType::None;
}

LocalityType IsLocalityChecker::GetType(FeatureType & f) const
{
  feature::TypesHolder types(f);
  return GetType(types);
}

IsCountryChecker::IsCountryChecker() : BaseCheckerEx({{"place", "country"}}) {}
IsStateChecker::IsStateChecker() : BaseCheckerEx({{"place", "state"}}) {}

IsCityTownOrVillageChecker::IsCityTownOrVillageChecker()
{
  base::StringIL const types[] = {{"place", "city"}, {"place", "town"}, {"place", "village"}, {"place", "hamlet"}};

  Classificator const & c = classif();
  for (auto const & e : types)
    m_types.push_back(c.GetTypeByPath(e));
}

IsEntranceChecker::IsEntranceChecker() : BaseCheckerEx({{"entrance"}}) {}
IsAerowayGateChecker::IsAerowayGateChecker() : BaseCheckerEx({{"aeroway", "gate"}}) {}
IsSubwayEntranceChecker::IsSubwayEntranceChecker() : BaseCheckerEx({{"railway", "subway_entrance"}}) {}

IsPlatformChecker::IsPlatformChecker()
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({"railway", "platform"}));
  m_types.push_back(c.GetTypeByPath({"public_transport", "platform"}));
}

IsAddressInterpolChecker::IsAddressInterpolChecker() : BaseChecker(1 /* level */)
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({"addr:interpolation"}));
  m_odd = c.GetTypeByPath({"addr:interpolation", "odd"});
  m_even = c.GetTypeByPath({"addr:interpolation", "even"});
}

uint64_t GetDefPopulation(LocalityType localityType)
{
  switch (localityType)
  {
  case LocalityType::Country: return 500000;
  case LocalityType::State: return 100000;
  case LocalityType::City: return 50000;
  case LocalityType::Town: return 10000;
  case LocalityType::Village: return 100;
  default: return 0;
  }
}

uint64_t GetPopulation(FeatureType & ft)
{
  uint64_t const p = ft.GetPopulation();
  return (p < 10 ? GetDefPopulation(IsLocalityChecker::Instance().GetType(ft)) : p);
}

double GetRadiusByPopulation(uint64_t p)
{
  return pow(static_cast<double>(p), 1 / 3.6) * 550.0;
}

// Look to: https://confluence.mail.ru/pages/viewpage.action?pageId=287950469
// for details about factors.
// Shortly, we assume: radius = (population ^ (1 / base)) * mult
// We knew area info about some cities|towns|villages and did grid search.
// Interval for base: [0.1, 100].
// Interval for mult: [10, 1000].
double GetRadiusByPopulationForRouting(uint64_t p, LocalityType localityType)
{
  switch (localityType)
  {
  case LocalityType::City: return pow(static_cast<double>(p), 1.0 / 2.5) * 34.0;
  case LocalityType::Town: return pow(static_cast<double>(p), 1.0 / 6.8) * 354.0;
  case LocalityType::Village: return pow(static_cast<double>(p), 1.0 / 15.1) * 610.0;
  default: UNREACHABLE();
  }
}

uint64_t GetPopulationByRadius(double r)
{
  return std::lround(pow(r / 550.0, 3.6));
}

}  // namespace ftypes
