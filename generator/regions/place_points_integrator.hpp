#pragma once

#include "generator/regions/country_specifier.hpp"
#include "generator/regions/node.hpp"
#include "generator/regions/place_point.hpp"

namespace generator
{
namespace regions
{
class PlacePointsIntegrator
{
public:
  PlacePointsIntegrator(PlacePointsMap const & placePoints,
      CountrySpecifier const & countrySpecifier);

  void ApplyTo(Node::PtrList & outers);

private:
  void ApplyLocalityPointsTo(Node::PtrList & outers);
  void ApplyLocalityPointTo(PlacePoint const & localityPoint, Node::PtrList & outers);
  void IntegrateLocalityPointInto(PlacePoint const & localityPoint, Node::Ptr & tree);
  
  PlacePointsMap const & m_placePoints;
  CountrySpecifier const & m_countrySpecifier;
};
}  // namespace regions
}  // namespace generator
