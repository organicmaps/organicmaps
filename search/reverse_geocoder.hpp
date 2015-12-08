#pragma once

#include "indexer/feature_decl.hpp"

#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"


class FeatureType;
class Index;

namespace search
{

class ReverseGeocoder
{
  Index & m_index;

public:
  ReverseGeocoder(Index & index) : m_index(index) {}

  struct Street
  {
    FeatureID m_id;

    /// Min distance to the street in meters.
    double m_distance;

    /// first - edit distance between actual street name and passed key name.
    /// second - length of the actual street name.
    pair<size_t, size_t> m_editDistance;
  };

  void GetNearbyStreets(FeatureType const & ft, string const & keyName, vector<Street> & streets);

  static size_t GetMatchedStreetIndex(vector<Street> const & streets);

private:
  template <class TCompare>
  void GetNearbyStreets(FeatureType const & ft, TCompare comp, vector<Street> & streets);
};

} // namespace search
