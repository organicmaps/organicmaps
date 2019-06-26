#include "generator/regions/place_points_integrator.hpp"

#include "generator/regions/locality_point_integrator.hpp"

namespace generator
{
namespace regions
{
PlacePointsIntegrator::PlacePointsIntegrator(PlacePointsMap const & placePoints,
    CountrySpecifier const & countrySpecifier)
  : m_placePoints(placePoints)
  , m_countrySpecifier{countrySpecifier}
{ }

void PlacePointsIntegrator::ApplyTo(Node::PtrList & outers)
{
  ApplyLocalityPointsTo(outers);
}

void PlacePointsIntegrator::ApplyLocalityPointsTo(Node::PtrList & outers)
{
  // Intergrate from biggest to smallest location types for intersections of nested localities.
  auto localityTypes = {PlaceType::City, PlaceType::Town,
                        PlaceType::Village, PlaceType::Hamlet, PlaceType::IsolatedDwelling};
  for (auto type : localityTypes)
  {
    for (auto const & placePoint : m_placePoints)
    {
      auto const & place = placePoint.second;
      if (place.GetPlaceType() == type)
        ApplyLocalityPointTo(place, outers);
    }
  }
}

void PlacePointsIntegrator::ApplyLocalityPointTo(PlacePoint const & localityPoint,
    Node::PtrList & outers)
{
  for (auto & tree : outers)
  {
    auto & countryRegion = tree->GetData();
    if (countryRegion.Contains(localityPoint))
    {
      IntegrateLocalityPointInto(localityPoint, tree);
      return;
    }
  }
}

void PlacePointsIntegrator::IntegrateLocalityPointInto(
    PlacePoint const & localityPoint, Node::Ptr & tree)
{
  auto localityIntegrator = LocalityPointIntegrator{localityPoint, m_countrySpecifier};
  if (!localityIntegrator.IntegrateInto(tree))
  {
    LOG(LWARNING, ("Can't integrate the",
                   StringifyPlaceType(localityPoint.GetPlaceType()), "place",
                   localityPoint.GetId(), "(", GetRegionNotation(localityPoint), ")",
                   "into", GetRegionNotation(tree->GetData())));
  }
}
}  // namespace regions
}  // namespace generator
