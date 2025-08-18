#include "drape/utils/projection.hpp"

namespace dp
{
std::array<float, 16> MakeProjection(dp::ApiVersion apiVersion, float left, float right, float bottom, float top)
{
  std::array<float, 16> result = {};

  // Projection matrix is calculated for [-1;1] depth-space, in some APIs (e.g. Metal)
  // depth-space is [0;1], so we have to remap projection matrix.
  float depthScale = 1.0f;
  float depthOffset = 0.0f;
  if (apiVersion == dp::ApiVersion::Metal)
  {
    depthScale = 0.5f;
    depthOffset = 0.5f;
  }

  float const width = right - left;
  float const height = top - bottom;
  float const depth = kMaxDepth - kMinDepth;

  result[0] = 2.0f / width;
  result[3] = -(right + left) / width;
  result[5] = 2.0f / height;
  result[7] = -(top + bottom) / height;
  result[10] = -2.0f * depthScale / depth;
  result[11] = depthOffset - (kMaxDepth + kMinDepth) / depth;
  result[15] = 1.0;

  return result;
}
}  // namespace dp
