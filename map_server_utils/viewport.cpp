#include "viewport.hpp"

#include "../indexer/mercator.hpp"

namespace srv
{
  namespace
  {
    inline double MercatorSizeX()
    {
      return MercatorBounds::maxX - MercatorBounds::minX;
    }

    inline double MercatorSizeY()
    {
      return MercatorBounds::maxY - MercatorBounds::minY;
    }
  }

  Viewport::Viewport(const m2::PointD & center, double scale, int width, int height)
    : m_center(center), m_scale(scale), m_width(width), m_height(height)
  {
  }

  const m2::PointD & Viewport::GetCenter() const
  {
    return m_center;
  }

  double Viewport::GetScale() const
  {
    return m_scale;
  }

  int Viewport::GetWidth() const
  {
    return m_width;
  }

  int Viewport::GetHeight() const
  {
    return m_height;
  }

  m2::AnyRectD Viewport::GetAnyRect() const
  {
    m2::RectD r(0, 0, MercatorSizeX() / pow(2, m_scale), MercatorSizeY() / pow(2, m_scale));
    m2::PointD pt(MercatorBounds::LonToX(m_center.x), MercatorBounds::LatToY(m_center.y));
    return m2::AnyRectD(pt, 0, m2::RectD(-r.SizeX() / 2, -r.SizeY() / 2, r.SizeX() / 2, r.SizeY() / 2));
  }
}
