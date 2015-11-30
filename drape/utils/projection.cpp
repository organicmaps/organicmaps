#include "projection.hpp"

namespace dp
{

void MakeProjection(array<float, 16> & result, float left, float right, float bottom, float top)
{
  result.fill(0.0f);

  float width = right - left;
  float height = top - bottom;
  float depth = maxDepth - minDepth;

  result[0] = 2.0f / width;
  result[3] = -(right + left) / width;
  result[5] = 2.0f / height;
  result[7] = -(top + bottom) / height;
  result[10] = -2.0f / depth;
  result[11] = -(maxDepth + minDepth) / depth;
  result[15] = 1.0;
}

} // namespace dp
