#include "navigator_utils.hpp"

namespace navi
{

m2::AnyRectD ToRotated(m2::RectD const & rect, Navigator const & navigator)
{
  double const dx = rect.SizeX();
  double const dy = rect.SizeY();

  return m2::AnyRectD(rect.Center(),
                      navigator.Screen().GetAngle(),
                      m2::RectD(-dx/2, -dy/2, dx/2, dy/2));
}

void SetRectFixedAR(m2::AnyRectD const & rect, const ScalesProcessor & scales, Navigator & navigator)
{
  double const halfSize = scales.GetTileSize() / 2.0;
  m2::RectD etalonRect(-halfSize, -halfSize, halfSize, halfSize);

  m2::PointD const pxCenter = navigator.Screen().PixelRect().Center();
  etalonRect.Offset(pxCenter);

  navigator.SetFromRects(rect, etalonRect);
}

} // namespace navi
