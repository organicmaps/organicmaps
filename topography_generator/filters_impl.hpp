#pragma once

#include "topography_generator/utils/values_provider.hpp"

#include "geometry/latlon.hpp"

#include <algorithm>
#include <vector>

namespace topography_generator
{
template <typename ValueType>
void GetExtendedTile(ms::LatLon const & leftBottom, size_t stepsInDegree,
                     size_t tileSize, size_t tileSizeExtension,
                     ValuesProvider<ValueType> & valuesProvider,
                     std::vector<ValueType> & extTileValues)
{
  size_t const extendedTileSize = tileSize + 2 * tileSizeExtension;
  extTileValues.resize(extendedTileSize * extendedTileSize);

  double const step = 1.0 / stepsInDegree;
  double const offset = step * tileSizeExtension;

  // Store values from North to South.
  ms::LatLon startPos = ms::LatLon(leftBottom.m_lat + 1.0 + offset,
                                   leftBottom.m_lon - offset);
  for (size_t i = 0; i < extendedTileSize; ++i)
  {
    for (size_t j = 0; j < extendedTileSize; ++j)
    {
      auto const pos = ms::LatLon(startPos.m_lat - i * step,
                                  startPos.m_lon + j * step);

      extTileValues[i * extendedTileSize + j] = valuesProvider.GetValue(pos);
    }
  }
}

template <typename ValueType>
void ProcessWithLinearKernel(std::vector<double> const & kernel, size_t tileSize, size_t tileOffset,
                             std::vector<ValueType> const & srcValues,
                             std::vector<ValueType> & dstValues)
{
  auto const kernelSize = kernel.size();
  auto const kernelRadius = kernel.size() / 2;
  CHECK_LESS_OR_EQUAL(kernelRadius, tileOffset, ());
  CHECK_GREATER(tileSize, tileOffset * 2, ());

  std::vector<ValueType> tempValues(tileSize, 0);

  for (size_t i = tileOffset; i < tileSize - tileOffset; ++i)
  {
    for (size_t j = tileOffset; j < tileSize - tileOffset; ++j)
    {
      tempValues[j] = 0.0;
      for (size_t k = 0; k < kernelSize; ++k)
      {
        size_t const srcIndex = i * tileSize + j - kernelRadius + k;
        tempValues[j] += kernel[k] * srcValues[srcIndex];
      }
    }
    for (size_t j = tileOffset; j < tileSize - tileOffset; ++j)
    {
      dstValues[i * tileSize + j] = tempValues[j];
    }
  }

  for (size_t j = tileOffset; j < tileSize - tileOffset; ++j)
  {
    for (size_t i = tileOffset; i < tileSize - tileOffset; ++i)
    {
      tempValues[i] = 0.0;
      for (size_t k = 0; k < kernelSize; ++k)
      {
        size_t const srcIndex = (i - kernelRadius + k) * tileSize + j;
        tempValues[i] += kernel[k] * dstValues[srcIndex];
      }
    }
    for (size_t i = tileOffset; i < tileSize - tileOffset; ++i)
    {
      dstValues[i * tileSize + j] = tempValues[i];
    }
  }
}

template <typename ValueType>
void ProcessWithSquareKernel(std::vector<double> const & kernel, size_t kernelSize,
                             size_t tileSize, size_t tileOffset,
                             std::vector<ValueType> const & srcValues,
                             std::vector<ValueType> & dstValues)
{
  CHECK_EQUAL(kernelSize * kernelSize, kernel.size(), ());
  size_t const kernelRadius = kernelSize / 2;
  CHECK_LESS_OR_EQUAL(kernelRadius, tileOffset, ());
  CHECK_GREATER(tileSize, tileOffset * 2, ());

  for (size_t i = tileOffset; i < tileSize - tileOffset; ++i)
  {
    for (size_t j = tileOffset; j < tileSize - tileOffset; ++j)
    {
      size_t const dstIndex = i * tileSize + j;
      dstValues[dstIndex] = 0;
      for (size_t ki = 0; ki < kernelSize; ++ki)
      {
        for (size_t kj = 0; kj < kernelSize; ++kj)
        {
          size_t const srcIndex = (i - kernelRadius + ki) * tileSize + j - kernelRadius + kj;
          dstValues[dstIndex] += kernel[ki * kernelSize + kj] * srcValues[srcIndex];
        }
      }
    }
  }
}

template <typename ValueType>
void ProcessMedian(size_t kernelRadius, size_t tileSize, size_t tileOffset,
                   std::vector<ValueType> const & srcValues,
                   std::vector<ValueType> & dstValues)
{
  CHECK_LESS_OR_EQUAL(kernelRadius, tileOffset, ());
  CHECK_GREATER(tileSize, tileOffset * 2, ());

  size_t const kernelSize = kernelRadius * 2 + 1;
  std::vector<ValueType> kernel(kernelSize * kernelSize);
  for (size_t i = tileOffset; i < tileSize - tileOffset; ++i)
  {
    for (size_t j = tileOffset; j < tileSize - tileOffset; ++j)
    {
      size_t const startI = i - kernelRadius;
      size_t const startJ = j - kernelRadius;
      for (size_t ki = 0; ki < kernelSize; ++ki)
      {
        for (size_t kj = 0; kj < kernelSize; ++kj)
        {
          size_t const srcIndex = (startI + ki) * tileSize + startJ + kj;
          kernel[ki * kernelSize + kj] = srcValues[srcIndex];
        }
      }
      std::sort(kernel.begin(), kernel.end());
      dstValues[i * tileSize + j] = kernel[kernelRadius];
    }
  }
}

// Calculate separable kernel for Gaussian blur. https://en.wikipedia.org/wiki/Gaussian_blur
std::vector<double> CalculateGaussianLinearKernel(double standardDeviation, double radiusFactor);
}  // namespace topography_generator
