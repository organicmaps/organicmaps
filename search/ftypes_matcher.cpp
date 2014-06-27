#include "ftypes_matcher.hpp"

#include "../indexer/feature.hpp"
#include "../indexer/feature_data.hpp"
#include "../indexer/classificator.hpp"


namespace ftypes
{

bool BaseChecker::IsMatched(uint32_t t) const
{
  ftype::TruncValue(t, 2);
  return (find(m_types.begin(), m_types.end(), t) != m_types.end());
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

IsBuildingChecker::IsBuildingChecker()
{
  Classificator const & c = classif();

  char const * arr0[] = { "building" };
  m_types.push_back(c.GetTypeByPath(vector<string>(arr0, arr0 + 1)));
  char const * arr1[] = { "building", "address" };
  m_types.push_back(c.GetTypeByPath(vector<string>(arr1, arr1 + 2)));
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
  static IsLocalityChecker inst;
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
