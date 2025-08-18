#pragma once

#include "topography_generator/utils/values_provider.hpp"

#include "geometry/latlon.hpp"

#include <algorithm>
#include <vector>

namespace topography_generator
{
template <typename ValueType>
void GetExtendedTile(ms::LatLon const & leftBottom, size_t stepsInDegree, size_t tileSize, size_t tileSizeExtension,
                     ValuesProvider<ValueType> & valuesProvider, std::vector<ValueType> & extTileValues)
{
  size_t const extendedTileSize = tileSize + 2 * tileSizeExtension;
  extTileValues.resize(extendedTileSize * extendedTileSize);

  double const step = 1.0 / stepsInDegree;
  double const offset = step * tileSizeExtension;

  // Store values from North to South.
  ms::LatLon startPos = ms::LatLon(leftBottom.m_lat + 1.0 + offset, leftBottom.m_lon - offset);
  for (size_t i = 0; i < extendedTileSize; ++i)
  {
    for (size_t j = 0; j < extendedTileSize; ++j)
    {
      auto pos = ms::LatLon(startPos.m_lat - i * step, startPos.m_lon + j * step);
      auto val = valuesProvider.GetValue(pos);

      if (val == valuesProvider.GetInvalidValue() && ((i < tileSizeExtension) || (i >= tileSizeExtension + tileSize) ||
                                                      (j < tileSizeExtension) || (j >= tileSizeExtension + tileSize)))
      {
        auto const ni = std::max(std::min(i, tileSizeExtension + tileSize - 1), tileSizeExtension);
        auto const nj = std::max(std::min(j, tileSizeExtension + tileSize - 1), tileSizeExtension);

        auto npos = ms::LatLon(startPos.m_lat - ni * step, startPos.m_lon + nj * step);
        val = valuesProvider.GetValue(npos);
      }
      extTileValues[i * extendedTileSize + j] = val;
    }
  }
}

template <typename ValueType>
void ProcessWithLinearKernel(std::vector<double> const & kernel, size_t tileSize, size_t tileOffset,
                             std::vector<ValueType> const & srcValues, std::vector<ValueType> & dstValues,
                             ValueType invalidValue)
{
  auto const kernelSize = kernel.size();
  auto const kernelRadius = kernel.size() / 2;
  CHECK_LESS_OR_EQUAL(kernelRadius, tileOffset, ());
  CHECK_GREATER(tileSize, tileOffset * 2, ());
  CHECK_EQUAL(dstValues.size(), tileSize * tileSize, ());

  std::vector<ValueType> tempValues(tileSize, 0);

  for (size_t i = tileOffset; i < tileSize - tileOffset; ++i)
  {
    for (size_t j = tileOffset; j < tileSize - tileOffset; ++j)
    {
      tempValues[j] = 0.0;
      auto const origValue = srcValues[i * tileSize + j];
      if (origValue == invalidValue)
      {
        tempValues[j] = invalidValue;
      }
      else
      {
        for (size_t k = 0; k < kernelSize; ++k)
        {
          size_t const srcIndex = i * tileSize + j - kernelRadius + k;
          auto srcValue = srcValues[srcIndex];
          if (srcValue == invalidValue)
            srcValue = origValue;
          tempValues[j] += kernel[k] * srcValue;
        }
      }
    }
    for (size_t j = tileOffset; j < tileSize - tileOffset; ++j)
      dstValues[i * tileSize + j] = tempValues[j];
  }

  for (size_t j = tileOffset; j < tileSize - tileOffset; ++j)
  {
    for (size_t i = tileOffset; i < tileSize - tileOffset; ++i)
    {
      tempValues[i] = 0.0;
      auto const origValue = dstValues[i * tileSize + j];
      if (origValue == invalidValue)
      {
        tempValues[i] = invalidValue;
      }
      else
      {
        for (size_t k = 0; k < kernelSize; ++k)
        {
          size_t const srcIndex = (i - kernelRadius + k) * tileSize + j;
          auto srcValue = dstValues[srcIndex];
          if (srcValue == invalidValue)
            srcValue = origValue;
          tempValues[i] += kernel[k] * srcValue;
        }
      }
    }
    for (size_t i = tileOffset; i < tileSize - tileOffset; ++i)
      dstValues[i * tileSize + j] = tempValues[i];
  }
}

template <typename ValueType>
void ProcessWithSquareKernel(std::vector<double> const & kernel, size_t kernelSize, size_t tileSize, size_t tileOffset,
                             std::vector<ValueType> const & srcValues, std::vector<ValueType> & dstValues,
                             ValueType invalidValue)
{
  CHECK_EQUAL(kernelSize * kernelSize, kernel.size(), ());
  size_t const kernelRadius = kernelSize / 2;
  CHECK_LESS_OR_EQUAL(kernelRadius, tileOffset, ());
  CHECK_GREATER(tileSize, tileOffset * 2, ());
  CHECK_EQUAL(dstValues.size(), tileSize * tileSize, ());

  for (size_t i = tileOffset; i < tileSize - tileOffset; ++i)
  {
    for (size_t j = tileOffset; j < tileSize - tileOffset; ++j)
    {
      size_t const dstIndex = i * tileSize + j;
      auto const origValue = srcValues[dstIndex];
      if (origValue == invalidValue)
      {
        dstValues[dstIndex] = invalidValue;
      }
      else
      {
        dstValues[dstIndex] = 0;
        for (size_t ki = 0; ki < kernelSize; ++ki)
        {
          for (size_t kj = 0; kj < kernelSize; ++kj)
          {
            size_t const srcIndex = (i - kernelRadius + ki) * tileSize + j - kernelRadius + kj;
            auto const srcValue = srcValues[srcIndex];
            if (srcValue == invalidValue)
              srcValue = origValue;
            dstValues[dstIndex] += kernel[ki * kernelSize + kj] * srcValue;
          }
        }
      }
    }
  }
}

template <typename ValueType>
void ProcessMedian(size_t kernelRadius, size_t tileSize, size_t tileOffset, std::vector<ValueType> const & srcValues,
                   std::vector<ValueType> & dstValues, ValueType invalidValue)
{
  CHECK_LESS_OR_EQUAL(kernelRadius, tileOffset, ());
  CHECK_GREATER(tileSize, tileOffset * 2, ());
  CHECK_EQUAL(dstValues.size(), tileSize * tileSize, ());

  size_t const kernelSize = kernelRadius * 2 + 1;
  std::vector<ValueType> kernel(kernelSize * kernelSize);
  for (size_t i = tileOffset; i < tileSize - tileOffset; ++i)
  {
    for (size_t j = tileOffset; j < tileSize - tileOffset; ++j)
    {
      size_t const dstIndex = i * tileSize + j;
      auto const origValue = srcValues[dstIndex];
      if (origValue == invalidValue)
      {
        dstValues[dstIndex] = invalidValue;
      }
      else
      {
        size_t const startI = i - kernelRadius;
        size_t const startJ = j - kernelRadius;
        for (size_t ki = 0; ki < kernelSize; ++ki)
        {
          for (size_t kj = 0; kj < kernelSize; ++kj)
          {
            size_t const srcIndex = (startI + ki) * tileSize + startJ + kj;
            auto srcValue = srcValues[srcIndex];
            if (srcValue == invalidValue)
              srcValue = origValue;
            kernel[ki * kernelSize + kj] = srcValue;
          }
        }
        std::sort(kernel.begin(), kernel.end());
        dstValues[dstIndex] = kernel[kernelRadius];
      }
    }
  }
}

// Calculate separable kernel for Gaussian blur. https://en.wikipedia.org/wiki/Gaussian_blur
std::vector<double> CalculateGaussianLinearKernel(double standardDeviation, double radiusFactor);
}  // namespace topography_generator
