/*
Copyright (C) 2009 by Stefan Gustavson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "drape/sdf_image.hpp"

#include "base/math.hpp"
#include "base/scope_guard.hpp"

#include "std/limits.hpp"
#include "std/bind.hpp"

namespace dp
{

namespace
{
  float const SQRT2 = 1.4142136f;

  float ComputeXGradient(float ul, float /*u*/, float ur, float l, float r, float dl, float /*d*/, float dr)
  {
    return (ur + SQRT2 * r + dr) - (ul + SQRT2 * l + dl);
  }

  float ComputeYGradient(float ul, float u, float ur, float /*l*/, float /*r*/, float dl, float d, float dr)
  {
    return (ur + SQRT2 * d + dr) - (ul + SQRT2 * u + dl);
  }

}
#define BIND_GRADIENT(f) bind(&f, _1, _2, _3, _4, _5, _6, _7, _8)
#define TRANSFORM(offset, dx, dy) \
  if (Transform(i, offset, dx, dy, xDist, yDist, oldDist)) \
  { \
    dist.m_data[i] = oldDist; \
    changed = true; \
  }


SdfImage::SdfImage(uint32_t h, uint32_t w)
  : m_height(h)
  , m_width(w)
{
  m_data.resize(m_width * m_height, 0);
}

SdfImage::SdfImage(uint32_t h, uint32_t w, uint8_t * imageData, uint8_t border)
{
  int8_t doubleBorder = 2 * border;
  m_width = w + doubleBorder;
  m_height = h + doubleBorder;

  uint32_t floatCount = m_width * m_height;
  m_data.resize(floatCount, 0.0f);
  for (size_t row = border; row < h + border; ++row)
  {
    size_t dstBaseIndex = row * m_width;
    size_t srcBaseIndex = (row - border) * w;
    for (size_t column = border; column < w + border; ++column)
      m_data[dstBaseIndex + column] = (float)imageData[srcBaseIndex + column - border] / 255.0f;
  }
}

SdfImage::SdfImage(SdfImage const & copy)
{
  m_height = copy.m_height;
  m_width = copy.m_width;
  m_data = copy.m_data;
}

uint32_t SdfImage::GetWidth() const
{
  return m_width;
}

uint32_t SdfImage::GetHeight() const
{
  return m_height;
}

void SdfImage::GetData(vector<uint8_t> & dst)
{
  ASSERT(m_data.size() <= dst.size(), ());
  transform(m_data.begin(), m_data.end(), dst.begin(), [](float const & node)
  {
    return static_cast<uint8_t>(node * 255.0f);
  });
}

void SdfImage::Scale()
{
  float maxi = numeric_limits<float>::min();
  float mini = numeric_limits<float>::max();

  for_each(m_data.begin(), m_data.end(), [&maxi, &mini](float const & node)
  {
    maxi = max(maxi, node);
    mini = min(mini, node);
  });

  maxi -= mini;
  for_each(m_data.begin(), m_data.end(), [&maxi, &mini](float & node)
  {
    node = (node - mini) / maxi;
  });
}

void SdfImage::Invert()
{
  for_each(m_data.begin(), m_data.end(), [](float & node)
  {
    node = 1.0f - node;
  });
}

void SdfImage::Minus(SdfImage & im)
{
  ASSERT(m_data.size() == im.m_data.size(), ());
  transform(m_data.begin(), m_data.end(), im.m_data.begin(), m_data.begin(), [](float const & n1, float const & n2)
  {
    return n1 - n2;
  });
}

void SdfImage::Distquant()
{
  for_each(m_data.begin(), m_data.end(), [](float & node)
  {
    node = my::clamp(0.5f + node * 0.0325f, 0.0f, 1.0f);
  });
}

void SdfImage::GenerateSDF(float sc)
{
  Scale();

  SdfImage outside(m_height, m_width);
  SdfImage inside(m_height, m_width);

  size_t shortCount = m_width * m_height;
  vector<short> xDist;
  vector<short> yDist;
  xDist.resize(shortCount, 0);
  yDist.resize(shortCount, 0);

  MexFunction(*this, xDist, yDist, outside);

  fill(xDist.begin(), xDist.end(), 0);
  fill(yDist.begin(), yDist.end(), 0);

  Invert();
  MexFunction(*this, xDist, yDist, inside);

  outside.Minus(inside);
  outside.Distquant();
  outside.Invert();
  *this = outside.Bilinear(sc);
}

SdfImage SdfImage::Bilinear(float scale)
{
  uint32_t srcWidth = GetWidth();
  uint32_t srcHeight = GetHeight();
  uint32_t dstWidth = srcWidth * scale;
  uint32_t dstHeight = srcHeight * scale;

  SdfImage result(dstHeight, dstWidth);

  float xRatio = static_cast<float>(srcWidth - 1) / result.GetWidth();
  float yRatio = static_cast<float>(srcHeight - 1) / result.GetHeight();
  for (uint32_t i = 0; i < dstHeight; i++)
  {
    uint32_t baseIndex = i * dstWidth;
    for (uint32_t j = 0; j < dstWidth; j++)
    {
      int x = static_cast<int>(xRatio * j);
      int y = static_cast<int>(yRatio * i);
      int index = y * srcWidth + x;
      ASSERT_LESS(index, m_data.size(), ());

      // range is 0 to 255 thus bitwise AND with 0xff
      float A = m_data[index];
      float B = m_data[index + 1];
      float C = m_data[index + srcWidth];
      float D = m_data[index + srcWidth + 1];

      float xDiff = (xRatio * j) - x;
      float yDiff = (yRatio * i) - y;
      float xInvertDiff = 1.0f - xDiff;
      float yInvertDiff = 1.0f - yDiff;

      float gray = A * xInvertDiff * yInvertDiff + B * xDiff * yInvertDiff +
                   C * xInvertDiff * yDiff + D * xDiff * yDiff;

      result.m_data[baseIndex + j] = gray;
    }
  }

  return result;
}

float SdfImage::ComputeGradient(uint32_t x, uint32_t y, SdfImage::TComputeFn const & fn) const
{
  if (x < 1 || x > m_width - 1 ||
      y < 1 || y > m_height - 1)
  {
    return 0.0;
  }

  size_t k = y * m_width + x;

  uint32_t l = k - 1;
  uint32_t r = k + 1;
  uint32_t u = k - m_width;
  uint32_t d = k + m_width;
  uint32_t ul = u - 1;
  uint32_t dl = d -1;
  uint32_t ur = u + 1;
  uint32_t dr = d + 1;

  if (m_data[k] > 0.0 && m_data[k] < 1.0)
  {
    return fn(m_data[ul], m_data[u], m_data[ur],
              m_data[l],             m_data[r],
              m_data[dl], m_data[d], m_data[dr]);
  }
  else
    return 0.0;
}

void SdfImage::MexFunction(SdfImage const & img, vector<short> & xDist, vector<short> & yDist, SdfImage & out)
{
  ASSERT_EQUAL(img.GetWidth(), out.GetWidth(), ());
  ASSERT_EQUAL(img.GetHeight(), out.GetHeight(), ());

  img.EdtaA3(xDist, yDist, out);
  // Pixels with grayscale>0.5 will have a negative distance.
  // This is correct, but we don't want values <0 returned here.
  for_each(out.m_data.begin(), out.m_data.end(), [](float & n)
  {
    n = max(0.0f, n);
  });
}

float SdfImage::DistaA3(int c, int xc, int yc, int xi, int yi) const
{
  int closest = c - xc - yc * m_width; // Index to the edge pixel pointed to from c
  //if (closest < 0 || closest > m_data.size())
  //  return 1000000.0;
  ASSERT_GREATER_OR_EQUAL(closest, 0, ());
  ASSERT_LESS(closest, m_data.size(), ());

  float a = my::clamp(m_data[closest], 0.0f, 1.0f); // Grayscale value at the edge pixel

  if(a == 0.0)
    return 1000000.0; // Not an object pixel, return "very far" ("don't know yet")

  double dx = static_cast<double>(xi);
  double dy = static_cast<double>(yi);
  double di = sqrt(dx * dx + dy * dy); // Length of integer vector, like a traditional EDT
  double df = 0.0;
  if(di == 0.0)
  {
    int y = closest / m_width;
    int x = closest % m_width;
    // Use local gradient only at edges
    // Estimate based on local gradient only
    df = EdgeDf(ComputeGradient(x, y, BIND_GRADIENT(ComputeXGradient)),
                ComputeGradient(x, y, BIND_GRADIENT(ComputeYGradient)), a);
  }
  else
  {
    // Estimate gradient based on direction to edge (accurate for large di)
    df = EdgeDf(dx, dy, a);
  }
  return static_cast<float>(di + df); // Same metric as edtaa2, except at edges (where di=0)
}

double SdfImage::EdgeDf(double gx, double gy, double a) const
{
  double df = 0.0;

  if ((gx == 0) || (gy == 0))
  {
    // Either A) gu or gv are zero
    //        B) both
    df = 0.5 - a;  // Linear approximation is A) correct or B) a fair guess
  }
  else
  {
    double glength = sqrt(gx * gx + gy * gy);
    if(glength > 0)
    {
      gx = gx / glength;
      gy = gy / glength;
    }

    // Everything is symmetric wrt sign and transposition,
    // so move to first octant (gx>=0, gy>=0, gx>=gy) to
    // avoid handling all possible edge directions.

    gx = fabs(gx);
    gy = fabs(gy);
    if (gx < gy)
      swap(gx, gy);

    double a1 = 0.5 * gy / gx;
    if (a < a1)
      df = 0.5 * (gx + gy) - sqrt(2.0 * gx * gy * a);
    else if (a < (1.0 - a1))
      df = (0.5 - a) * gx;
    else
      df = -0.5 * (gx + gy) + sqrt(2.0 * gx * gy * (1.0 - a));
  }

  return df;
}

void SdfImage::EdtaA3(vector<short> & xDist, vector<short> & yDist, SdfImage & dist) const
{
  ASSERT_EQUAL(dist.GetHeight(), GetHeight(), ());
  ASSERT_EQUAL(dist.GetWidth(), GetWidth(), ());
  ASSERT_EQUAL(dist.m_data.size(), m_data.size(), ());

  int w = GetWidth();
  int h = GetHeight();

  /* Initialize the distance SdfImages */
  for (size_t y = 0; y < h; ++y)
  {
    size_t baseIndex = y * w;
    for (size_t x = 0; x < w; ++x)
    {
      size_t index = baseIndex + x;
      if (m_data[index] <= 0.0)
        dist.m_data[index]= 1000000.0; // Big value, means "not set yet"
      else if (m_data[index] < 1.0)
      {
        dist.m_data[index] = EdgeDf(ComputeGradient(x, y, BIND_GRADIENT(ComputeXGradient)),
                                    ComputeGradient(x, y, BIND_GRADIENT(ComputeYGradient)),
                                    m_data[index]);
      }
    }
  }

  /* Initialize index offsets for the current SdfImage width */
  int offsetU = -w;
  int offsetD = w;
  int offsetR = 1;
  int offsetL = -1;
  int offsetRu = -w + 1;
  int offsetRd = w + 1;
  int offsetLd = w - 1;
  int offsetLu = -w - 1;

  /* Perform the transformation */
  bool changed;
  do
  {
    changed = false;
    for(int y = 1; y < h; ++y)
    {
      int i = y * w;

      /* scan right, propagate distances from above & left */
      /* Leftmost pixel is special, has no left neighbors */
      float oldDist = dist.m_data[i];
      if(oldDist > 0) // If non-zero distance or not set yet
      {
        TRANSFORM(offsetU, 0, 1);
        TRANSFORM(offsetRu, -1, 1);
      }

      ++i;

      /* Middle pixels have all neighbors */
      for(int x = 1; x < w - 1; ++x, ++i)
      {
        oldDist = dist.m_data[i];
        if(oldDist > 0.0)
        {
          TRANSFORM(offsetL, 1, 0);
          TRANSFORM(offsetLu, 1, 1);
          TRANSFORM(offsetU, 0, 1);
          TRANSFORM(offsetRu, -1, 1);
        }
      }

      /* Rightmost pixel of row is special, has no right neighbors */
      oldDist = dist.m_data[i];
      if(oldDist > 0)
      {
        TRANSFORM(offsetL, 1, 0);
        TRANSFORM(offsetLu, 1, 1);
        TRANSFORM(offsetU, 0, 1);
      }

      /* Move index to second rightmost pixel of current row. */
      /* Rightmost pixel is skipped, it has no right neighbor. */
      i = y * w + w - 2;

      /* scan left, propagate distance from right */
      for(int x = w - 2; x >= 0; --x, --i)
      {
        oldDist = dist.m_data[i];
        if(oldDist > 0.0)
          TRANSFORM(offsetR, -1, 0);
      }
    }

    /* Scan rows in reverse order, except last row */
    for(int y = h - 2; y >= 0; --y)
    {
      /* move index to rightmost pixel of current row */
      int i = y * w + w - 1;

      /* Scan left, propagate distances from below & right */

      /* Rightmost pixel is special, has no right neighbors */
      float oldDist = dist.m_data[i];
      if(oldDist > 0) // If not already zero distance
      {
        TRANSFORM(offsetD, 0, -1);
        TRANSFORM(offsetLd, 1, -1);
      }

      --i;

      /* Middle pixels have all neighbors */
      for(int x = w - 2; x > 0; --x, --i)
      {
        oldDist = dist.m_data[i];
        if(oldDist > 0.0)
        {
          TRANSFORM(offsetR, -1, 0);
          TRANSFORM(offsetRd, -1, -1);
          TRANSFORM(offsetD, 0, -1);
          TRANSFORM(offsetLd, 1, -1);
        }
      }

      /* Leftmost pixel is special, has no left neighbors */
      oldDist = dist.m_data[i];
      if(oldDist > 0)
      {
        TRANSFORM(offsetR, -1, 0);
        TRANSFORM(offsetRd, -1, -1);
        TRANSFORM(offsetD, 0, -1);
      }

      /* Move index to second leftmost pixel of current row. */
      /* Leftmost pixel is skipped, it has no left neighbor. */
      i = y * w + 1;
      for(int x = 1; x < w; ++x, ++i)
      {
        /* scan right, propagate distance from left */
        oldDist = dist.m_data[i];
        if(oldDist > 0.0)
          TRANSFORM(offsetL, 1, 0);
      }
    }
  }
  while(changed);
}

bool SdfImage::Transform(int baseIndex, int offset, int dx, int dy, vector<short> & xDist, vector<short> & yDist, float & oldDist) const
{
  double const epsilon = 1e-3;
  ASSERT_EQUAL(xDist.size(), yDist.size(), ());
  ASSERT_GREATER_OR_EQUAL(baseIndex, 0, ());
  ASSERT_LESS(baseIndex, xDist.size(), ());

  int candidate = baseIndex + offset;
  ASSERT_GREATER_OR_EQUAL(candidate, 0, ());
  ASSERT_LESS(candidate, xDist.size(), ());

  int cDistX = xDist[candidate];
  int cDistY = yDist[candidate];
  int newDistX = cDistX + dx;
  int newDistY = cDistY + dy;
  float newDist = DistaA3(candidate, cDistX, cDistY, newDistX, newDistY);
  if(newDist < oldDist - epsilon)
  {
    xDist[baseIndex] = newDistX;
    yDist[baseIndex] = newDistY;
    oldDist = newDist;
    return true;
  }

  return false;
}

} // namespace dp
