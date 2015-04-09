#include "graphics/coordinates.hpp"

namespace graphics
{
  void getOrthoMatrix(math::Matrix<float, 4, 4> & m,
                      float l,
                      float r,
                      float b,
                      float t,
                      float n,
                      float f)
  {
    m(0, 0) = 2 / (r - l); m(0, 1) = 0;          m(0, 2) = 0;            m(0, 3) = -(r + l) / (r - l);
    m(1, 0) = 0;           m(1, 1) = 2 / (t - b);m(1, 2) = 0;            m(1, 3) = -(t + b) / (t - b);
    m(2, 0) = 0;           m(2, 1) = 0;          m(2, 2) = -2 / (f - n);  m(2, 3) = (f + n) / (f - n);
    m(3, 0) = 0;           m(3, 1) = 0;          m(3, 2) = 0;            m(3, 3) = 1;
  }
}
