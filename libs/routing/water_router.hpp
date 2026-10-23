#pragma once

#include "routing/router.hpp"
#include "routing/water_geometry.hpp"

namespace routing
{
class WaterRouter final : public IRouter
{
public:
  // Runs on the routing worker. The provider must preserve topology and certify extraction completeness.
  using GeometryGetter = std::function<water::Geometry(Checkpoints const &, RouterDelegate const &)>;

  explicit WaterRouter(GeometryGetter geometryGetter) : m_geometryGetter(std::move(geometryGetter)) {}

  std::string GetName() const override { return "water-router"; }
  void ClearState() override {}
  void SetGuides(GuidesTracks &&) override {}
  bool FindClosestProjectionToRoad(m2::PointD const &, m2::PointD const &, double, EdgeProj &) override
  {
    return false;
  }
  RouterResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & startDirection,
                                  bool adjustToPrevRoute, bool needAlternatives, RouterDelegate const & delegate,
                                  RoutesResult & result) override;

private:
  GeometryGetter m_geometryGetter;
};
}  // namespace routing
