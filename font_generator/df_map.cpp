#include "df_map.hpp"

#include "../std/cmath.hpp"

DFMap::DFMap(const vector<unsigned char> & data, int width, int height, unsigned char inValue, unsigned char outValue)
  : m_width(width)
  , m_height(height)
{
  //m_distances = new float[m_width * m_height];
  m_distances.resize(m_width * m_height);
  for (int i = 0; i < m_width * m_height; ++i)
    m_distances[i] = 0.0f;

  Do(data, inValue, outValue);
}

DFMap::~DFMap()
{
}

void DFMap::Minus(DFMap const & other)
{
  for (int j = 0; j < m_height; ++j)
    for (int i = 0; i < m_width; ++i)
    {
      float srcD = m_distances[CalcIndex(i, j)];
      float dstD = other.m_distances[CalcIndex(i, j)];
      SetDistance(srcD - dstD, i, j);
    }
}

void DFMap::Normalize()
{
  float minSignedDistance = 0;
  for (int j = 0; j < m_height; ++j)
    for (int i = 0; i < m_width; ++i)
      minSignedDistance = fmin(minSignedDistance, GetDistance(i, j));

  float maxValue = 0.0f;
  for (int j = 0; j < m_height; ++j)
    for (int i = 0; i < m_width; ++i)
    {
      float d = GetDistance(i, j) - minSignedDistance;
      maxValue = fmax(d, maxValue);
      SetDistance(d, i, j);
    }

  for (int j = 0; j < m_height; ++j)
    for (int i = 0; i < m_width; ++i)
      SetDistance(GetDistance(i, j) / maxValue, i, j);
}

unsigned char * DFMap::GenerateImage(int & w, int & h)
{
  w = m_width;
  h = m_height;
  unsigned char * image = new unsigned char[m_width * m_height];
  for (int j = 0; j < m_height; ++j)
    for (int i = 0; i < m_width; ++i)
    {
      unsigned char v = 255 * GetDistance(i, j);
      put(image, v, i, j);
    }

  return image;
}

void DFMap::Do(vector<unsigned char> const & data, unsigned char inValue, unsigned char outValue)
{
  for (int j = 0; j < m_height; ++j)
  {
    for (int i = 0; i < m_width; ++i)
    {
      if (get(data, i, j) == inValue)
        SetDistance(sqrtf(findRadialDistance(data, i, j, 256, outValue)), i, j);
    }
  }
}

float DFMap::findRadialDistance(vector<unsigned char> const & data, int pointX, int pointY, int maxRadius, unsigned char outValue)
{
  float minDistance = maxRadius * maxRadius;
  for (int j = pointY - maxRadius; j < pointY + maxRadius; ++j)
  {
    if (j < 0 || j >= m_height)
      continue;

    for (int i = pointX - maxRadius; i < pointX + maxRadius; ++i)
    {
      if (i < 0 || i >= m_width)
        continue;

      if (get(data, i, j) == outValue)
      {
        int xDist = pointX - i;
        int yDist = pointY - j;
        float d = xDist * xDist + yDist * yDist;
        minDistance = fmin(d, minDistance);
      }
    }
  }

  return minDistance;
}

float DFMap::GetDistance(int i, int j) const
{
  return get(m_distances, i, j);
}

void DFMap::SetDistance(float val, int i, int j)
{
  put(m_distances, val, i, j);
}
