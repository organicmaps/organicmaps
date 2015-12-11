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
  static double const kLookupRadiusM;

  ReverseGeocoder(Index & index) : m_index(index) {}

  struct Street
  {
    FeatureID m_id;
    double m_distanceMeters;
    string m_name;
  };

  void GetNearbyStreets(FeatureType const & addrFt, vector<Street> & streets);

  static size_t GetMatchedStreetIndex(string const & keyName, vector<Street> const & streets);

private:
  template <class TCompare>
  void GetNearbyStreets(FeatureType const & ft, TCompare comp, vector<Street> & streets);
};

} // namespace search
