#include "indexer/ftypes_matcher.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/classificator.hpp"

#include "std/sstream.hpp"
#include "std/utility.hpp"

namespace ftypes
{

uint32_t BaseChecker::PrepareToMatch(uint32_t type, uint8_t level)
{
  ftype::TruncValue(type, level);
  return type;
}

bool BaseChecker::IsMatched(uint32_t type) const
{
  return (find(m_types.begin(), m_types.end(), PrepareToMatch(type, m_level)) != m_types.end());
}

bool BaseChecker::operator() (feature::TypesHolder const & types) const
{
  for (uint32_t t : types)
    if (IsMatched(t))
      return true;

  return false;
}

bool BaseChecker::operator() (FeatureType const & ft) const
{
  return this->operator() (feature::TypesHolder(ft));
}

bool BaseChecker::operator() (vector<uint32_t> const & types) const
{
  for (size_t i = 0; i < types.size(); ++i)
  {
    if (IsMatched(types[i]))
      return true;
  }
  return false;
}

IsPeakChecker::IsPeakChecker()
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({ "natural", "peak" }));
}

IsPeakChecker const & IsPeakChecker::Instance()
{
  static const IsPeakChecker inst;
  return inst;
}


IsATMChecker::IsATMChecker()
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({ "amenity", "atm" }));
}

IsATMChecker const & IsATMChecker::Instance()
{
  static const IsATMChecker inst;
  return inst;
}

IsSpeedCamChecker::IsSpeedCamChecker()
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({ "highway", "speed_camera"}));
}

IsSpeedCamChecker const & IsSpeedCamChecker::Instance()
{
  static const IsSpeedCamChecker inst;
  return inst;
}

IsFuelStationChecker::IsFuelStationChecker()
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({ "amenity", "fuel" }));
}

IsFuelStationChecker const & IsFuelStationChecker::Instance()
{
  static const IsFuelStationChecker inst;
  return inst;
}


IsStreetChecker::IsStreetChecker()
{
  Classificator const & c = classif();
  char const * arr[][2] = {
    { "highway", "trunk" },
    { "highway", "primary" },
    { "highway", "secondary" },
    { "highway", "residential" },
    { "highway", "pedestrian" },
    { "highway", "tertiary" },
    { "highway", "construction" },
    { "highway", "living_street" },
    { "highway", "service" },
    { "highway", "unclassified" }
  };

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
    m_types.push_back(c.GetTypeByPath(vector<string>(arr[i], arr[i] + 2)));
}

IsStreetChecker const & IsStreetChecker::Instance()
{
  static const IsStreetChecker inst;
  return inst;
}

IsOneWayChecker::IsOneWayChecker()
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({ "hwtag", "oneway" }));
}

IsOneWayChecker const & IsOneWayChecker::Instance()
{
  static const IsOneWayChecker inst;
  return inst;
}

IsRoundAboutChecker::IsRoundAboutChecker()
{
  Classificator const & c = classif();
  m_types.push_back(c.GetTypeByPath({ "junction", "roundabout" }));
}

IsRoundAboutChecker const & IsRoundAboutChecker::Instance()
{
  static const IsRoundAboutChecker inst;
  return inst;
}

IsLinkChecker::IsLinkChecker()
{
  Classificator const & c = classif();
  char const * arr[][2] = {
    { "highway", "motorway_link" },
    { "highway", "trunk_link" },
    { "highway", "primary_link" },
    { "highway", "secondary_link" },
    { "highway", "tertiary_link" }
  };

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
    m_types.push_back(c.GetTypeByPath(vector<string>(arr[i], arr[i] + 2)));
}

IsLinkChecker const & IsLinkChecker::Instance()
{
  static const IsLinkChecker inst;
  return inst;
}

IsBuildingChecker::IsBuildingChecker()
{
  Classificator const & c = classif();

  m_types.push_back(c.GetTypeByPath({ "building" }));
  m_types.push_back(c.GetTypeByPath({ "building", "address" }));
}

IsBuildingChecker const & IsBuildingChecker::Instance()
{
  static const IsBuildingChecker inst;
  return inst;
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

IsBridgeChecker::IsBridgeChecker() : BaseChecker(3)
{ 
}

IsBridgeChecker const & IsBridgeChecker::Instance()
{
  static const IsBridgeChecker inst;
  return inst;
}

bool IsBridgeChecker::IsMatched(uint32_t type) const
{
  return IsTypeConformed(type, {"highway", "*", "bridge"});
}

IsTunnelChecker::IsTunnelChecker() : BaseChecker(3)
{
}

IsTunnelChecker const & IsTunnelChecker::Instance()
{
  static const IsTunnelChecker inst;
  return inst;
}

bool IsTunnelChecker::IsMatched(uint32_t type) const
{
  return IsTypeConformed(type, {"highway", "*", "tunnel"});
}

Type IsLocalityChecker::GetType(feature::TypesHolder const & types) const
{
  for (uint32_t t : types)
  {
    ftype::TruncValue(t, 2);

    size_t j = COUNTRY;
    for (; j < LOCALITY_COUNT; ++j)
      if (t == m_types[j])
        return static_cast<Type>(j);

    for (; j < m_types.size(); ++j)
      if (t == m_types[j])
        return VILLAGE;
  }

  return NONE;
}

Type IsLocalityChecker::GetType(const FeatureType & f) const
{
  feature::TypesHolder types(f);
  return GetType(types);
}

IsLocalityChecker const & IsLocalityChecker::Instance()
{
  static IsLocalityChecker const inst;
  return inst;
}

uint32_t GetPopulation(FeatureType const & ft)
{
  uint32_t population = ft.GetPopulation();

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

double GetRadiusByPopulation(uint32_t p)
{
  return pow(static_cast<double>(p), 0.277778) * 550.0;
}

uint32_t GetPopulationByRadius(double r)
{
  return my::rounds(pow(r / 550.0, 3.6));
}

bool IsTypeConformed(uint32_t type, vector<string> const & path)
{
  Classificator const & c = classif();
  ClassifObject const * p = c.GetRoot();
  ASSERT(p, ());

  uint8_t val = 0, i = 0;
  for (auto const n : path)
  {
    if (!ftype::GetValue(type, i, val))
      return false;
    p = p->GetObject(val);
    if (p == 0)
      return false;
    string const name = p->GetName();
    if (n != name && n != "*")
      return false;
    ++i;
  }
  return true;
}

string DebugPrint(HighwayClass const cls)
{
  stringstream out;
  out << "[ ";
  switch (cls)
  {
    case HighwayClass::Undefined:
      out << "Undefined";
    case HighwayClass::Error:
      out << "Error";
    case HighwayClass::Trunk:
      out << "Trunk";
    case HighwayClass::Primary:
      out << "Primary";
    case HighwayClass::Secondary:
      out << "Secondary";
    case HighwayClass::Tertiary:
      out << "Tertiary";
    case HighwayClass::LivingStreet:
      out << "LivingStreet";
    case HighwayClass::Service:
      out << "Service";
    case HighwayClass::Count:
      out << "Count";
    default:
      out << "Unknown value of HighwayClass: " << static_cast<int>(cls);
  }
  out << " ]";
  return out.str();
}

HighwayClass GetHighwayClass(feature::TypesHolder const & types)
{
  Classificator const & c = classif();
  static pair<HighwayClass, uint32_t> const kHighwayClasses[] = {
      {HighwayClass::Trunk, c.GetTypeByPath({"highway", "motorway"})},
      {HighwayClass::Trunk, c.GetTypeByPath({"highway", "motorway_link"})},
      {HighwayClass::Trunk, c.GetTypeByPath({"highway", "trunk"})},
      {HighwayClass::Trunk, c.GetTypeByPath({"highway", "trunk_link"})},
      {HighwayClass::Primary, c.GetTypeByPath({"highway", "primary"})},
      {HighwayClass::Primary, c.GetTypeByPath({"highway", "primary_link"})},
      {HighwayClass::Secondary, c.GetTypeByPath({"highway", "secondary"})},
      {HighwayClass::Secondary, c.GetTypeByPath({"highway", "secondary_link"})},
      {HighwayClass::Tertiary, c.GetTypeByPath({"highway", "tertiary"})},
      {HighwayClass::Tertiary, c.GetTypeByPath({"highway", "tertiary_link"})},
      {HighwayClass::LivingStreet, c.GetTypeByPath({"highway", "unclassified"})},
      {HighwayClass::LivingStreet, c.GetTypeByPath({"highway", "residential"})},
      {HighwayClass::LivingStreet, c.GetTypeByPath({"highway", "living_street"})},
      {HighwayClass::Service, c.GetTypeByPath({"highway", "service"})},
      {HighwayClass::Service, c.GetTypeByPath({"highway", "track"})}};
  uint8_t const kTruncLevel = 2;

  for (auto t : types)
  {
    ftype::TruncValue(t, kTruncLevel);
    for (auto const & cls : kHighwayClasses)
    {
      if (cls.second == t)
        return cls.first;
    }
  }

  return HighwayClass::Error;
}

HighwayClass GetHighwayClass(FeatureType const & ft)
{
  return GetHighwayClass(feature::TypesHolder(ft));
}
}
