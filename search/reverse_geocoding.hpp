#pragma once

#include "indexer/feature_decl.hpp"

#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"


class FeatureType;
class Index;

namespace search
{

class ReverseGeocoding
{
  Index * m_index;

public:
  ReverseGeocoding(Index * p) : m_index(p) {}

  struct Street
  {
    FeatureID m_id;
    double m_distance;
    pair<size_t, size_t> m_editDistance;
  };

  void GetNearbyStreets(FeatureType const & ft, string const & keyName, vector<Street> & streets);

  static size_t GetMatchedStreetIndex(vector<Street> const & streets);

private:
  template <class TCompare>
  void GetNearbyStreets(FeatureType const & ft, TCompare comp, vector<Street> & streets);
};

} // namespace search
