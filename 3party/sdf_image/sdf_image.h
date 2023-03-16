#pragma once

// +----------------------------------------+
// |                                        |
// |   http://contourtextures.wikidot.com   |
// |                                        |
// +----------------------------------------+

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

#include "base/buffer_vector.hpp"

#include <cstdint>
#include <functional>
#include <vector>

namespace sdf_image
{
class SdfImage
{
public:
  SdfImage() = default;
  SdfImage(uint32_t h, uint32_t w);
  SdfImage(uint32_t h, uint32_t w, uint8_t * imageData, uint8_t border);
  SdfImage(SdfImage const & copy);

  uint32_t GetWidth() const;
  uint32_t GetHeight() const;
  void GetData(std::vector<uint8_t> & dst);
  void GenerateSDF(float sc);

private:
  void Scale();
  void Invert();
  void Minus(SdfImage &im);
  void Distquant();
  SdfImage Bilinear(float Scale);

private:
  /// ul = up left
  /// u = up
  /// ...
  /// d = down
  /// dr = down right
  ///                       ul    u      ur     l       r     dl      d     dr
  using TComputeFn = std::function<float (float, float, float, float, float, float, float, float)>;
  float ComputeGradient(uint32_t x, uint32_t y, TComputeFn const & fn) const;
  void MexFunction(SdfImage const & img, std::vector<short> & xDist, std::vector<short> & yDist,
                   SdfImage & out);
  float DistaA3(int c, int xc, int yc, int xi, int yi) const;
  double EdgeDf(double gx, double gy, double a) const;
  void EdtaA3(std::vector<short> & xDist, std::vector<short> & yDist, SdfImage & dist) const;
  bool Transform(int baseIndex, int offset, int dx, int dy, std::vector<short> & xDist,
                 std::vector<short> & yDist, float & oldDist) const;

private:
  uint32_t m_height = 0;
  uint32_t m_width = 0;
  buffer_vector<float, 512> m_data;
};
}  // namespace sdf_image
