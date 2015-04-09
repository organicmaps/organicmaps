#include "base/matrix.hpp"

namespace graphics
{
  /// Calculate matrix for orthographic projection
  void getOrthoMatrix(math::Matrix<float, 4, 4> & m,
                      float left,
                      float right,
                      float bottom,
                      float top,
                      float near,
                      float far);
}
