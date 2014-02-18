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

}
