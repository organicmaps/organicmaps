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

  explicit WaterRouter(GeometryGetter geometryGetter, std::unique_ptr<IRouter> pedestrian = {},
                       GeometryGetter expandedGeometryGetter = {})
    : m_geometryGetter(std::move(geometryGetter))
    , m_pedestrian(std::move(pedestrian))
    , m_expandedGeometryGetter(std::move(expandedGeometryGetter))
  {}

  std::string GetName() const override { return "water-router"; }
  void ClearState() override
  {
    if (m_pedestrian)
      m_pedestrian->ClearState();
  }
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
  std::unique_ptr<IRouter> m_pedestrian;
  GeometryGetter m_expandedGeometryGetter;
};
}  // namespace routing
