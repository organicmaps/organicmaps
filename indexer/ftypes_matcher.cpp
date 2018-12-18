#include "indexer/ftypes_matcher.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"

#include "base/assert.hpp"
#include "base/buffer_vector.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <map>
#include <sstream>
#include <unordered_map>

using namespace std;

namespace
{
class HighwayClasses
{
  map<uint32_t, ftypes::HighwayClass> m_map;

public:
  HighwayClasses()
  {
    auto const & c = classif();
    m_map[c.GetTypeByPath({"route", "ferry"})] = ftypes::HighwayClass::Transported;
    m_map[c.GetTypeByPath({"route", "shuttle_train"})] = ftypes::HighwayClass::Transported;
    m_map[c.GetTypeByPath({"railway", "rail"})] = ftypes::HighwayClass::Transported;

    m_map[c.GetTypeByPath({"highway", "motorway"})] = ftypes::HighwayClass::Trunk;
    m_map[c.GetTypeByPath({"highway", "motorway_link"})] = ftypes::HighwayClass::Trunk;
    m_map[c.GetTypeByPath({"highway", "trunk"})] = ftypes::HighwayClass::Trunk;
    m_map[c.GetTypeByPath({"highway", "trunk_link"})] = ftypes::HighwayClass::Trunk;

    m_map[c.GetTypeByPath({"highway", "primary"})] = ftypes::HighwayClass::Primary;
    m_map[c.GetTypeByPath({"highway", "primary_link"})] = ftypes::HighwayClass::Primary;

    m_map[c.GetTypeByPath({"highway", "secondary"})] = ftypes::HighwayClass::Secondary;
    m_map[c.GetTypeByPath({"highway", "secondary_link"})] = ftypes::HighwayClass::Secondary;

    m_map[c.GetTypeByPath({"highway", "tertiary"})] = ftypes::HighwayClass::Tertiary;
    m_map[c.GetTypeByPath({"highway", "tertiary_link"})] = ftypes::HighwayClass::Tertiary;

    m_map[c.GetTypeByPath({"highway", "unclassified"})] = ftypes::HighwayClass::LivingStreet;
    m_map[c.GetTypeByPath({"highway", "residential"})] = ftypes::HighwayClass::LivingStreet;
    m_map[c.GetTypeByPath({"highway", "living_street"})] = ftypes::HighwayClass::LivingStreet;
    m_map[c.GetTypeByPath({"highway", "road"})] = ftypes::HighwayClass::LivingStreet;

    m_map[c.GetTypeByPath({"highway", "service"})] = ftypes::HighwayClass::Service;
    m_map[c.GetTypeByPath({"highway", "track"})] = ftypes::HighwayClass::Service;
    m_map[c.GetTypeByPath({"man_made", "pier"})] = ftypes::HighwayClass::Service;

    m_map[c.GetTypeByPath({"highway", "pedestrian"})] = ftypes::HighwayClass::Pedestrian;
    m_map[c.GetTypeByPath({"highway", "footway"})] = ftypes::HighwayClass::Pedestrian;
    m_map[c.GetTypeByPath({"highway", "bridleway"})] = ftypes::HighwayClass::Pedestrian;
    m_map[c.GetTypeByPath({"highway", "steps"})] = ftypes::HighwayClass::Pedestrian;
    m_map[c.GetTypeByPath({"highway", "cycleway"})] = ftypes::HighwayClass::Pedestrian;
    m_map[c.GetTypeByPath({"highway", "path"})] = ftypes::HighwayClass::Pedestrian;
  }

  ftypes::HighwayClass Get(uint32_t t) const
  {
    auto const it = m_map.find(t);
    if (it == m_map.cend())
      return ftypes::HighwayClass::Error;
    return it->second;
  }
};

char const * HighwayClassToString(ftypes::HighwayClass const cls)
{
  switch (cls)
  {
  case ftypes::HighwayClass::Undefined: return "Undefined";
  case ftypes::HighwayClass::Error: return "Error";
  case ftypes::HighwayClass::Transported: return "Transported";
  case ftypes::HighwayClass::Trunk: return "Trunk";
  case ftypes::HighwayClass::Primary: return "Primary";
  case ftypes::HighwayClass::Secondary: return "Secondary";
  case ftypes::HighwayClass::Tertiary: return "Tertiary";
  case ftypes::HighwayClass::LivingStreet: return "LivingStreet";
  case ftypes::HighwayClass::Service: return "Service";
  case ftypes::HighwayClass::Pedestrian: return "Pedestrian";
  case ftypes::HighwayClass::Count: return "Count";
  }
  ASSERT(false, ());
  return "";
}
}  // namespace

namespace ftypes
{
string DebugPrint(HighwayClass const cls)
{
  stringstream out;
  out << "[ " << HighwayClassToString(cls) << " ]";
  return out.str();
}

HighwayClass GetHighwayClass(feature::TypesHolder const & types)
{
  uint8_t constexpr kTruncLevel = 2;
  static HighwayClasses highwayClasses;

  for (auto t : types)
  {
    ftype::TruncValue(t, kTruncLevel);
    HighwayClass const hc = highwayClasses.Get(t);
    if (hc != HighwayClass::Error)
      return hc;
  }

  return HighwayClass::Error;
}

uint32_t BaseChecker::PrepareToMatch(uint32_t type, uint8_t level)
{
  ftype::TruncValue(type, level);
  return type;
}

bool BaseChecker::IsMatched(uint32_t type) const
{
  return (find(m_types.begin(), m_types.end(), PrepareToMatch(type, m_level)) != m_types.end());
}

bool BaseChecker::operator()(feature::TypesHolder const & types) const
{
  for (uint32_t t : types)
  {
    if (IsMatched(t))
      return true;
  }

  return false;
}

bool BaseChecker::operator()(FeatureType & ft) const
{
  return this->operator()(feature::TypesHolder(ft));
}

bool BaseChecker::operator()(vector<uint32_t> const & types) const
{
  for (uint32_t t : types)
  {
    if (IsMatched(t))
      return true;
  }

  return false;
}

IsPeakChecker::IsPeakChecker()
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({"natural", "peak"}));
}

IsATMChecker::IsATMChecker()
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({"amenity", "atm"}));
}

IsSpeedCamChecker::IsSpeedCamChecker()
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({"highway", "speed_camera"}));
}

IsFuelStationChecker::IsFuelStationChecker()
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({"amenity", "fuel"}));
}

IsRailwayStationChecker::IsRailwayStationChecker()
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({"railway", "station"}));
  m_types.push_back(c.GetTypeByPath({"building", "train_station"}));
}

IsSubwayStationChecker::IsSubwayStationChecker() : BaseChecker(3 /* level */)
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({"railway", "station", "subway"}));
}

IsAirportChecker::IsAirportChecker()
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({"aeroway", "aerodrome"}));
}

IsStreetChecker::IsStreetChecker()
{
  // TODO (@y, @m, @vng): this list must be up-to-date with
  // data/categories.txt, so, it's worth it to generate or parse it
  // from that file.
  Classificator const & c = classif();
  char const * arr[][2] = {{"highway", "living_street"},
                           {"highway", "footway"},
                           {"highway", "motorway"},
                           {"highway", "motorway_link"},
                           {"highway", "path"},
                           {"highway", "pedestrian"},
                           {"highway", "primary"},
                           {"highway", "primary_link"},
                           {"highway", "residential"},
                           {"highway", "road"},
                           {"highway", "secondary"},
                           {"highway", "secondary_link"},
                           {"highway", "service"},
                           {"highway", "tertiary"},
                           {"highway", "tertiary_link"},
                           {"highway", "track"},
                           {"highway", "trunk"},
                           {"highway", "trunk_link"},
                           {"highway", "unclassified"}};
  for (auto const & p : arr)
    m_types.push_back(c.GetTypeByPath({p[0], p[1]}));
}

IsAddressObjectChecker::IsAddressObjectChecker() : BaseChecker(1 /* level */)
{
  auto const paths = {"building", "amenity", "shop", "tourism", "historic", "office", "craft"};

  Classificator const & c = classif();
  for (auto const & p : paths)
    m_types.push_back(c.GetTypeByPath({p}));
}

IsVillageChecker::IsVillageChecker()
{
  // TODO (@y, @m, @vng): this list must be up-to-date with
  // data/categories.txt, so, it's worth it to generate or parse it
  // from that file.
  Classificator const & c = classif();
  char const * arr[][2] = {{"place", "village"}, {"place", "hamlet"}};

  for (auto const & p : arr)
    m_types.push_back(c.GetTypeByPath({p[0], p[1]}));
}

IsOneWayChecker::IsOneWayChecker()
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({"hwtag", "oneway"}));
}

IsRoundAboutChecker::IsRoundAboutChecker()
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({"junction", "roundabout"}));
}

IsLinkChecker::IsLinkChecker()
{
  Classificator const & c = classif();
  char const * arr[][2] = {{"highway", "motorway_link"},
                           {"highway", "trunk_link"},
                           {"highway", "primary_link"},
                           {"highway", "secondary_link"},
                           {"highway", "tertiary_link"}};

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
    m_types.push_back(c.GetTypeByPath(vector<string>(arr[i], arr[i] + 2)));
}

IsBuildingChecker::IsBuildingChecker() : BaseChecker(1 /* level */)
{
  m_types.push_back(classif().GetTypeByPath({"building"}));
}

// static
set<string> const IsPoiChecker::kPoiTypes = {
  "amenity",
  "shop",
  "tourism",
  "leisure",
  "sport",
  "craft",
  "man_made",
  "emergency",
  "office",
  "historic",
  "railway",
  "highway",
  "aeroway"
};

IsPoiChecker::IsPoiChecker() : BaseChecker(1 /* level */)
{
  for (auto const & type : IsPoiChecker::kPoiTypes)
    m_types.push_back(classif().GetTypeByPath({type}));
}

// static
set<pair<string, string>> const WikiChecker::kTypesForWiki = {
  {"amenity", "place_of_worship"},
  {"historic", "archaeological_site"},
  {"historic", "castle"},
  {"historic", "memorial"},
  {"historic", "monument"},
  {"historic", "museum"},
  {"historic", "ruins"},
  {"historic", "ship"},
  {"historic", "tomb"},
  {"tourism", "artwork"},
  {"tourism", "attraction"},
  {"tourism", "museum"},
  {"tourism", "gallery"},
  {"tourism", "viewpoint"},
  {"tourism", "zoo"},
  {"tourism", "theme_park"},
  {"leisure", "park"},
  {"leisure", "water_park"},
  {"highway", "pedestrian"},
  {"man_made", "lighthouse"},
  {"waterway", "waterfall"},
  {"leisure", "garden"},
};

WikiChecker::WikiChecker() : BaseChecker(2 /* level */)
{
  for (auto const & t : kTypesForWiki)
    m_types.push_back(classif().GetTypeByPath({t.first, t.second}));
}

IsPlaceChecker::IsPlaceChecker() : BaseChecker(1 /* level */)
{
  m_types.push_back(classif().GetTypeByPath({"place"}));
}

IsBridgeChecker::IsBridgeChecker() : BaseChecker(3 /* level */) {}

bool IsBridgeChecker::IsMatched(uint32_t type) const
{
  return IsTypeConformed(type, {"highway", "*", "bridge"});
}

IsTunnelChecker::IsTunnelChecker() : BaseChecker(3 /* level */) {}

bool IsTunnelChecker::IsMatched(uint32_t type) const
{
  return IsTypeConformed(type, {"highway", "*", "tunnel"});
}

IsHotelChecker::IsHotelChecker()
{
  Classificator const & c = classif();
  for (size_t i = 0; i < static_cast<size_t>(Type::Count); ++i)
  {
    auto const hotelType = static_cast<Type>(i);
    auto const * const tag = GetHotelTypeTag(hotelType);
    auto const type = c.GetTypeByPath({"tourism", tag});

    m_types.push_back(type);

    m_sortedTypes[i].first = type;
    m_sortedTypes[i].second = hotelType;
  }

  sort(m_sortedTypes.begin(), m_sortedTypes.end());
}

unsigned IsHotelChecker::GetHotelTypesMask(FeatureType & ft) const
{
  feature::TypesHolder types(ft);
  buffer_vector<uint32_t, feature::kMaxTypesCount> sortedTypes(types.begin(), types.end());
  sort(sortedTypes.begin(), sortedTypes.end());

  unsigned mask = 0;
  size_t i = 0;
  size_t j = 0;
  while (i < sortedTypes.size() && j < m_sortedTypes.size())
  {
    if (sortedTypes[i] < m_sortedTypes[j].first)
    {
      ++i;
    }
    else if (sortedTypes[i] > m_sortedTypes[j].first)
    {
      ++j;
    }
    else
    {
      mask |= 1U << static_cast<unsigned>(m_sortedTypes[j].second);
      ++i;
      ++j;
    }
  }

  return mask;
}

IsPopularityPlaceChecker::IsPopularityPlaceChecker()
{
  vector<pair<string, string>> const popularityPlaceTypes = {
    {"amenity", "bar"},
    {"amenity", "biergarten"},
    {"amenity", "cafe"},
    {"amenity", "casino"},
    {"amenity", "cinema"},
    {"amenity", "fast_food"},
    {"amenity", "fountain"},
    {"amenity", "grave_yard"},
    {"amenity", "marketplace"},
    {"amenity", "nightclub"},
    {"amenity", "place_of_worship"},
    {"amenity", "pub"},
    {"amenity", "restaurant"},
    {"amenity", "theatre"},
    {"amenity", "townhall"},
    {"highway", "pedestrian"},
    {"historic", "archaeological_site"},
    {"historic", "castle"},
    {"historic", "memorial"},
    {"historic", "monument"},
    {"historic", "museum"},
    {"historic", "ruins"},
    {"historic", "ship"},
    {"historic", "tomb"},
    {"landuse", "cemetery"},
    {"leisure", "garden"},
    {"leisure", "park"},
    {"leisure", "water_park"},
    {"man_made", "lighthouse"},
    {"natural", "geyser"},
    {"natural", "peak"},
    {"shop", "bakery"},
    {"tourism", "artwork"},
    {"tourism", "attraction"},
    {"tourism", "gallery"},
    {"tourism", "museum"},
    {"tourism", "theme_park"},
    {"tourism", "viewpoint"},
    {"tourism", "zoo"},
    {"waterway", "waterfall"}
  };

  Classificator const & c = classif();
  for (auto const & t : popularityPlaceTypes)
    m_types.push_back(c.GetTypeByPath({t.first, t.second}));
}

boost::optional<IsHotelChecker::Type> IsHotelChecker::GetHotelType(FeatureType & ft) const
{
  feature::TypesHolder types(ft);
  buffer_vector<uint32_t, feature::kMaxTypesCount> sortedTypes(types.begin(), types.end());
  sort(sortedTypes.begin(), sortedTypes.end());

  size_t i = 0;
  size_t j = 0;
  while (i < sortedTypes.size() && j < m_sortedTypes.size())
  {
    if (sortedTypes[i] < m_sortedTypes[j].first)
      ++i;
    else if (sortedTypes[i] > m_sortedTypes[j].first)
      ++j;
    else
      return m_sortedTypes[j].second;
  }
  return {};
}

// static
char const * IsHotelChecker::GetHotelTypeTag(Type type)
{
  switch (type)
  {
  case Type::Hotel: return "hotel";
  case Type::Apartment: return "apartment";
  case Type::CampSite: return "camp_site";
  case Type::Chalet: return "chalet";
  case Type::GuestHouse: return "guest_house";
  case Type::Hostel: return "hostel";
  case Type::Motel: return "motel";
  case Type::Resort: return "resort";
  case Type::Count: CHECK(false, ("Can't get hotel type tag")); return "";
  }
  UNREACHABLE();
}

IsWifiChecker::IsWifiChecker()
{
  m_types.push_back(classif().GetTypeByPath({"internet_access", "wlan"}));
}

IsEatChecker::IsEatChecker()
{
  Classificator const & c = classif();
  map<Type, vector<string>> const descriptions = {{Type::Cafe,       {"amenity", "cafe"}},
                                                  {Type::Bakery,     {"shop", "bakery"}},
                                                  {Type::FastFood,   {"amenity", "fast_food"}},
                                                  {Type::Restaurant, {"amenity", "restaurant"}},
                                                  {Type::Bar,        {"amenity", "bar"}},
                                                  {Type::Pub,        {"amenity", "pub"}},
                                                  {Type::Biergarten, {"amenity", "biergarten"}}};

  for (auto const & desc : descriptions)
  {
    auto const type = c.GetTypeByPath(desc.second);
    m_types.push_back(type);
    m_sortedTypes[static_cast<size_t>(desc.first)] = {type, desc.first};
  }
}

IsEatChecker::Type IsEatChecker::GetType(uint32_t t) const
{
  for (auto type : m_sortedTypes)
  {
    if (type.first == t)
      return type.second;
  }

  return IsEatChecker::Type::Count;
}

IsCuisineChecker::IsCuisineChecker() : BaseChecker(1 /* level */)
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({"cuisine"}));
}

IsCityChecker::IsCityChecker()
{
  m_types.push_back(classif().GetTypeByPath({"place", "city"}));
}

IsPublicTransportStopChecker::IsPublicTransportStopChecker()
{
  m_types.push_back(classif().GetTypeByPath({"highway", "bus_stop"}));
  m_types.push_back(classif().GetTypeByPath({"railway", "tram_stop"}));
}

IsLocalityChecker::IsLocalityChecker()
{
  Classificator const & c = classif();

  // Note! The order should be equal with constants in Type enum (add other villages to the end).
  char const * arr[][2] = {
    { "place", "country" },
    { "place", "state" },
    { "place", "city" },
    { "place", "town" },
    { "place", "village" },
    { "place", "hamlet" }
  };

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
    m_types.push_back(c.GetTypeByPath(vector<string>(arr[i], arr[i] + 2)));
}

Type IsLocalityChecker::GetType(uint32_t t) const
{
  ftype::TruncValue(t, 2);

  size_t j = COUNTRY;
  for (; j < LOCALITY_COUNT; ++j)
    if (t == m_types[j])
      return static_cast<Type>(j);

  for (; j < m_types.size(); ++j)
    if (t == m_types[j])
      return VILLAGE;

  return NONE;
}

Type IsLocalityChecker::GetType(feature::TypesHolder const & types) const
{
  for (uint32_t t : types)
  {
    Type const type = GetType(t);
    if (type != NONE)
      return type;
  }
  return NONE;
}

Type IsLocalityChecker::GetType(FeatureType & f) const
{
  feature::TypesHolder types(f);
  return GetType(types);
}

uint64_t GetPopulation(FeatureType & ft)
{
  uint64_t population = ft.GetPopulation();

  if (population < 10)
  {
    switch (IsLocalityChecker::Instance().GetType(ft))
    {
    case CITY:
    case TOWN:
      population = 10000;
      break;
    case VILLAGE:
      population = 100;
      break;
    default:
      population = 0;
    }
  }

  return population;
}

double GetRadiusByPopulation(uint64_t p)
{
  return pow(static_cast<double>(p), 0.277778) * 550.0;
}

uint64_t GetPopulationByRadius(double r)
{
  return base::rounds(pow(r / 550.0, 3.6));
}

bool IsTypeConformed(uint32_t type, base::StringIL const & path)
{
  ClassifObject const * p = classif().GetRoot();
  ASSERT(p, ());

  uint8_t val = 0, i = 0;
  for (char const * s : path)
  {
    if (!ftype::GetValue(type, i, val))
      return false;

    p = p->GetObject(val);
    if (p == 0)
      return false;

    if (p->GetName() != s && strcmp(s, "*") != 0)
      return false;

    ++i;
  }
  return true;
}
}  // namespace ftypes
