#pragma once

#include "generator/boost_helpers.hpp"
#include "generator/regions/country_specifier.hpp"
#include "generator/regions/level_region.hpp"
#include "generator/regions/node.hpp"
#include "generator/regions/place_point.hpp"

#include <memory>

namespace generator
{
namespace regions
{
// LocalityPointIntegrator inserts place point (city/town/village) into region tree
// as region with around boundary or boundary from administrative region (by name matching).
class LocalityPointIntegrator
{
public:
  LocalityPointIntegrator(PlacePoint const & localityPoint,
      CountrySpecifier const & countrySpecifier);

  bool IntegrateInto(Node::Ptr & tree);

private:
  bool IsSuitableForLocalityRegionize(LevelRegion const & region) const;
  bool HasIntegratingLocalityName(LevelRegion const & region) const;
  void EnlargeRegion(LevelRegion & region);
  void RegionizeBy(LevelRegion const & region);
  void InsertInto(Node::Ptr & node);
  void EmboundBy(LevelRegion const & region);
  static LevelRegion MakeAroundRegion(PlacePoint const & localityPoint,
                                      CountrySpecifier const & countrySpecifier);
  // This function uses heuristics and assigns a radius according to the tag place.
  // The radius will be returned in mercator units.
  static double GetRadiusByPlaceType(PlaceType place);
  static std::shared_ptr<BoostPolygon> MakePolygonWithRadius(
      BoostPoint const & point, double radius, size_t numPoints = 16);

  LevelRegion m_localityRegion;
  CountrySpecifier const & m_countrySpecifier;
  boost::optional<LevelRegion> m_regionizedByRegion;
};
}  // namespace regions
}  // namespace generator
