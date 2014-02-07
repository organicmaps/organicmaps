#include "ftypes_matcher.hpp"

#include "../indexer/feature.hpp"
#include "../indexer/feature_data.hpp"
#include "../indexer/classificator.hpp"


namespace ftypes
{

bool BaseChecker::operator() (feature::TypesHolder const & types) const
{
  for (size_t i = 0; i < types.Size(); ++i)
  {
    uint32_t t = types[i];
    ftype::TruncValue(t, 2);

    if (find(m_types.begin(), m_types.end(), t) != m_types.end())
      return true;
  }
  return false;
}

bool BaseChecker::operator() (FeatureType const & ft) const
{
  return this->operator() (feature::TypesHolder(ft));
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
    { "highway", "living_street" }
  };

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
    m_types.push_back(c.GetTypeByPath(vector<string>(arr[i], arr[i] + 2)));
}

}
