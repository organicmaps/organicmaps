#include "topography_generator/filters_impl.hpp"

namespace topography_generator
{
std::vector<double> CalculateGaussianLinearKernel(double standardDeviation, double radiusFactor)
{
  auto const kernelRadius = static_cast<int64_t>(ceil(radiusFactor * standardDeviation));
  auto const kernelSize = 2 * kernelRadius + 1;
  std::vector<double> linearKernel(kernelSize, 0);

  double sum = 1.0;
  linearKernel[kernelRadius] = 1.0;
  for (int64_t i = 1; i <= kernelRadius; ++i)
  {
    double const val = exp(-i * i / (2 * standardDeviation * standardDeviation));
    linearKernel[kernelRadius - i] = linearKernel[kernelRadius + i] = val;
    sum += 2.0 * val;
  }
  for (auto & val : linearKernel)
    val /= sum;

  return linearKernel;
}
}  // namespace topography_generator
