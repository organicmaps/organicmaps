#include "drape_frontend/animation/modelview_center_animation.hpp"
#include "drape_frontend/animation/interpolations.hpp"

namespace df
{

ModelViewCenterAnimation::ModelViewCenterAnimation(m2::PointD const & start,
                                                 m2::PointD const & end,
                                                 double duration)
  : BaseViewportAnimation(duration)
  , m_startPt(start)
  , m_endPt(end)
{
}

void ModelViewCenterAnimation::Apply(Navigator & navigator)
{
  m2::PointD center = Interpolate(m_startPt, m_endPt, GetT());
  navigator.CenterViewport(center);
}

}
