#pragma once

#include <string>
#include "routing/router.hpp"

namespace routing
{

class RulerRouter : public IRouter
{
  std::string GetName() const override { return {"ruler-router"}; }
  void ClearState() override;
  void SetGuides(GuidesTracks && guides) override;
  RouterResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & startDirection,
                                  bool adjustToPrevRoute, RouterDelegate const & delegate, Route & route) override;
  bool FindClosestProjectionToRoad(m2::PointD const & point, m2::PointD const & direction, double radius,
                                   EdgeProj & proj) override;

  // Do we need guides in this router?
  // GuidesConnections m_guides;
};
}  // namespace routing
