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

  struct Object
  {
    FeatureID m_id;
    double m_distanceMeters;
    string m_name;

    Object() = default;
    Object(FeatureID const & id, double dist, string const & name)
      : m_id(id), m_distanceMeters(dist), m_name(name)
    {
    }
  };

public:
  static double const kLookupRadiusM;
  static m2::RectD GetLookupRect(m2::PointD const & center);

  explicit ReverseGeocoder(Index & index) : m_index(index) {}

  using Street = Object;

  struct Building : public Object
  {
    m2::PointD m_center;

    Building() = default;
    Building(FeatureID const & id, double dist, string const & hn, m2::PointD const & center)
      : Object(id, dist, hn), m_center(center)
    {
    }
  };

  void GetNearbyStreets(FeatureType const & addrFt, vector<Street> & streets);

  static size_t GetMatchedStreetIndex(string const & keyName, vector<Street> const & streets);

  void GetNearbyAddress(m2::PointD const & center,
                        Building & building, Street & street);

private:
  void GetNearbyStreets(m2::PointD const & center, vector<Street> & streets);
  void GetNearbyBuildings(m2::PointD const & center, vector<Building> & buildings);
};

} // namespace search
