#include "df_map.hpp"

#include "../base/math.hpp"

#include "../std/algorithm.hpp"
#include "../std/cmath.hpp"

#include <boost/gil/algorithm.hpp>

using boost::gil::gray8c_pixel_t;
using boost::gil::gray8_view_t;
using boost::gil::gray8_pixel_t;
using boost::gil::interleaved_view;

DFMap::DFMap(vector<uint8_t> const & data,
             int32_t width, int32_t height,
             uint8_t inValue, uint8_t outValue)
  : m_width(width)
  , m_height(height)
{
  m_distances.resize(m_width * m_height);
  for (int32_t i = 0; i < m_width * m_height; ++i)
    m_distances[i] = 0.0f;

  gray8c_view_t srcView = interleaved_view(width, height,
                                           (gray8c_pixel_t *)&data[0],
                                           width);

  Do(srcView, inValue, outValue);
}

DFMap::~DFMap()
{
}

void DFMap::Minus(DFMap const & other)
{
  for (int32_t j = 0; j < m_height; ++j)
    for (int32_t i = 0; i < m_width; ++i)
    {
      float const srcD = m_distances[CalcIndex(i, j)];
      float const dstD = other.m_distances[CalcIndex(i, j)];
      SetDistance(srcD - dstD, i, j);
    }
}

void DFMap::Normalize()
{
  float minSignedDistance = 0;
  for (int j = 0; j < m_height; ++j)
    for (int i = 0; i < m_width; ++i)
      minSignedDistance = min(minSignedDistance, GetDistance(i, j));

  float maxValue = 0.0f;
  for (int j = 0; j < m_height; ++j)
    for (int i = 0; i < m_width; ++i)
    {
      float d = GetDistance(i, j) - minSignedDistance;
      maxValue = max(d, maxValue);
      SetDistance(d, i, j);
    }

  for (int j = 0; j < m_height; ++j)
    for (int i = 0; i < m_width; ++i)
      SetDistance(GetDistance(i, j) / maxValue, i, j);
}

void DFMap::GenerateImage(vector<uint8_t> & image, int32_t & w, int32_t & h)
{
  w = m_width;
  h = m_height;
  image.resize(m_width * m_height);
  gray8_view_t view = interleaved_view(m_width, m_height,
                                       (gray8_pixel_t *)&image[0], m_width);
  for (gray8_view_t::y_coord_t y = 0; y < view.height(); ++y)
  {
    for (gray8_view_t::x_coord_t x = 0; x < view.width(); ++x)
    {
      view(x, y) = my::clamp(192 * GetDistance(x, y), 0, 255);
    }
  }
}

void DFMap::Do(gray8c_view_t const & view, uint8_t inValue, uint8_t outValue)
{
  for (gray8_view_t::y_coord_t y = 0; y < view.height(); ++y)
  {
    for (gray8_view_t::x_coord_t x = 0; x < view.width(); ++x)
    {
      if (view(x, y) == inValue)
        SetDistance(sqrt(findRadialDistance(view, x, y, 256, outValue)), x, y);
    }
  }
}

namespace
{
  class SquareGoRoundIterator
  {
  public:
    SquareGoRoundIterator(gray8c_view_t const & view,
                          int32_t centerX, int32_t centerY, uint32_t r)
      : m_view(view)
      , m_state(ToRight)
    {
      m_startX = centerX - r;
      m_startY = centerY - r;
      m_rWidth = m_rHeight = (2 * r) + 1;
      if (m_startX < 0)
      {
        m_rWidth += m_startX;
        m_startX = 0;
      }
      else if (m_startX + m_rWidth >= m_view.width())
      {
        m_rWidth = (m_view.width() - 1) - m_startX;
      }

      if (m_startY < 0)
      {
        m_rHeight += m_startY;
        m_startY = 0;
      }
      else if (m_startY + m_rHeight >= m_view.width())
      {
        m_rHeight = (m_view.width() - 1) - m_startY;
      }

      m_currentX = m_startX;
      m_currentY = m_startY;
    }

    bool HasValue() const { return m_state != End; }
    uint8_t GetValue() const { return m_view(m_currentX, m_currentY); }
    int32_t GetXMetric() const { return m_currentX; }
    int32_t GetYMetric() const { return m_currentY; }

    void Next()
    {
      MoveForward();
      if (m_currentX < 0 || m_currentX >= m_startX + m_rWidth ||
          m_currentY < 0 || m_currentY >= m_startY + m_rHeight)
      {
        MoveBackward();
        ChangeMoveType();
        MoveForward();
      }
    }

  private:
    void MoveForward()
    {
      switch (m_state)
      {
      case ToRight:
        m_currentX += 1;
        break;
      case ToLeft:
        m_currentX -= 1;
        break;
      case ToBottom:
        m_currentY += 1;
        break;
      case ToTop:
        m_currentY -= 1;
        break;
      default:
        break;
      }
    }

    void MoveBackward()
    {
      switch (m_state)
      {
      case ToRight:
        m_currentX -= 1;
        break;
      case ToLeft:
        m_currentX += 1;
        break;
      case ToBottom:
        m_currentY -= 1;
        break;
      case ToTop:
        m_currentY += 1;
        break;
      default:
        break;
      }
    }

    void ChangeMoveType()
    {
      switch (m_state)
      {
      case ToRight:
        m_state = ToBottom;
        break;
      case ToBottom:
        m_state = ToLeft;
        break;
      case ToLeft:
        m_state = ToTop;
        break;
      case ToTop:
        m_state = End;
        break;
      default:
        break;
      }
    }

  private:
    gray8c_view_t const & m_view;

    enum
    {
      ToRight,
      ToBottom,
      ToLeft,
      ToTop,
      End
    } m_state;

    int32_t m_startX, m_startY;
    int32_t m_currentX, m_currentY;
    int32_t m_rWidth, m_rHeight;
  };
}

float DFMap::findRadialDistance(gray8c_view_t const & view,
                                int32_t pointX, int32_t pointY,
                                int32_t maxRadius, uint8_t outValue) const
{
  int32_t minDistance = maxRadius * maxRadius;
  for (int32_t r = 1; (r < maxRadius) && (r * r) < minDistance; ++r)
  {
    SquareGoRoundIterator iter(view, pointX, pointY, r);
    while (iter.HasValue())
    {
      uint8_t v = iter.GetValue();
      if (v == outValue)
      {
        int32_t xDist = pointX - iter.GetXMetric();
        int32_t yDist = pointY - iter.GetYMetric();
        int32_t d = xDist * xDist + yDist * yDist;
        minDistance = min(d, minDistance);
      }
      iter.Next();
    }
  }

  return minDistance;
}

float DFMap::GetDistance(int32_t i, int32_t j) const
{
  return get(m_distances, i, j);
}

void DFMap::SetDistance(float val, int32_t i, int32_t j)
{
  put(m_distances, val, i, j);
}
