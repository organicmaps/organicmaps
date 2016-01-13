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
  Index const & m_index;

  struct Object
  {
    FeatureID m_id;
    double m_distanceMeters;
    string m_name;

    Object() : m_distanceMeters(0.0) {}
    Object(FeatureID const & id, double dist, string const & name)
      : m_id(id), m_distanceMeters(dist), m_name(name)
    {
    }

    inline bool IsValid() const { return m_id.IsValid(); }
  };

public:
  static double const kLookupRadiusM;

  explicit ReverseGeocoder(Index const & index);

  using Street = Object;

  struct Building : public Object
  {
    m2::PointD m_center;

    // To investigate possible errors.
    // There are no houses in (0, 0) coordinates.
    Building() : m_center(0, 0) {}

    Building(FeatureID const & id, double dist, string const & number, m2::PointD const & center)
      : Object(id, dist, number), m_center(center)
    {
    }
  };

  void GetNearbyStreets(FeatureType const & addrFt, vector<Street> & streets) const;

  static size_t GetMatchedStreetIndex(string const & keyName, vector<Street> const & streets);

  struct Address
  {
    Building m_building;
    Street m_street;

    string GetHouseNumber() const { return m_building.m_name; }
    string GetStreetName() const { return m_street.m_name; }
  };

  void GetNearbyAddress(m2::PointD const & center, Address & addr) const;

  /// @returns street segments (can be duplicate names) sorted by distance to feature's center.
  /// uint32_t, if less than vector.size(), contains index of exact feature's street specified in OSM data.
  pair<vector<Street>, uint32_t> GetNearbyFeatureStreets(FeatureType const & feature) const;

  void GetNearbyBuildings(m2::PointD const & center, vector<Building> & buildings) const;

  void GetNearbyBuildings(m2::PointD const & center, double radiusM, vector<Building> & buildings) const;

private:
  static m2::RectD GetLookupRect(m2::PointD const & center, double radiusM);

  void GetNearbyStreets(m2::PointD const & center, vector<Street> & streets) const;
};

} // namespace search
