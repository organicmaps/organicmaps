#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "geometry/polyline2d.hpp"

namespace df
{

class RouteShape
{
public:
  RouteShape(m2::PolylineD const & polyline,
             CommonViewParams const & params);

  void Draw(ref_ptr<dp::Batcher> batcher) const;

private:
  CommonViewParams m_params;
  m2::PolylineD m_polyline;
};

} // namespace df
