#include "ftypes_matcher.hpp"

#include "../indexer/feature.hpp"
#include "../indexer/feature_data.hpp"
#include "../indexer/classificator.hpp"


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
  for (size_t i = 0; i < types.Size(); ++i)
  {
    if (IsMatched(types[i]))
      return true;
  }
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

IsBridgeChecker::IsBridgeChecker() : BaseChecker(3), m_typeMask(0)
{ 
  Classificator const & c = classif();
  uint32_t type = c.GetTypeByPath({"highway", "road", "bridge"});
  uint8_t highwayType, bridgeType;
  ftype::GetValue(type, 0, highwayType);
  ftype::GetValue(type, 2, bridgeType);

  size_t const recordSz = 3;
  char const * arr[][recordSz] = {
    { "highway", "living_street", "bridge" },
    { "highway", "motorway", "bridge" },
    { "highway", "motorway_link", "bridge" },
    { "highway", "primary", "bridge" },
    { "highway", "primary_link", "bridge" },
    { "highway", "residential", "bridge" },
    { "highway", "road", "bridge" },
    { "highway", "secondary", "bridge" },
    { "highway", "secondary_link", "bridge" },
    { "highway", "service", "bridge" },
    { "highway", "tertiary", "bridge" },
    { "highway", "tertiary_link", "bridge" },
    { "highway", "trunk", "bridge" },
    { "highway", "trunk_link", "bridge" },
    { "highway", "unclassified", "bridge" }
  };

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
    m_types.push_back(c.GetTypeByPath(vector<string>(arr[i], arr[i] + recordSz)));
}

IsBridgeChecker const & IsBridgeChecker::Instance()
{
  static const IsBridgeChecker inst;
  return inst;
}

IsTunnelChecker::IsTunnelChecker() : BaseChecker(3), m_typeMask(0)
{
  Classificator const & c = classif();
  size_t const recordSz = 3;
  char const * arr[][recordSz] = {
    { "highway", "living_street", "tunnel" },
    { "highway", "motorway", "tunnel" },
    { "highway", "motorway_link", "tunnel" },
    { "highway", "primary", "tunnel" },
    { "highway", "primary_link", "tunnel" },
    { "highway", "residential", "tunnel" },
    { "highway", "road", "tunnel" },
    { "highway", "secondary", "tunnel" },
    { "highway", "secondary_link", "tunnel" },
    { "highway", "service", "tunnel" },
    { "highway", "tertiary", "tunnel" },
    { "highway", "tertiary_link", "tunnel" },
    { "highway", "trunk", "tunnel" },
    { "highway", "trunk_link", "tunnel" },
    { "highway", "unclassified", "tunnel" }
  };

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
    m_types.push_back(c.GetTypeByPath(vector<string>(arr[i], arr[i] + recordSz)));
}

IsTunnelChecker const & IsTunnelChecker::Instance()
{
  static const IsTunnelChecker inst;
  return inst;
}

Type IsLocalityChecker::GetType(feature::TypesHolder const & types) const
{
  for (size_t i = 0; i < types.Size(); ++i)
  {
    uint32_t t = types[i];
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

}
