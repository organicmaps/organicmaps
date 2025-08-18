#pragma once

#include "topography_generator/filters_impl.hpp"

namespace topography_generator
{
template <typename ValueType>
class FilterInterface
{
public:
  virtual ~FilterInterface() = default;

  virtual size_t GetKernelRadius() const = 0;
  virtual void Process(size_t tileSize, size_t tileOffset, std::vector<ValueType> const & srcValues,
                       std::vector<ValueType> & dstValues, ValueType invalidValue) const = 0;
};

template <typename ValueType>
class MedianFilter : public FilterInterface<ValueType>
{
public:
  explicit MedianFilter(size_t kernelRadius) : m_kernelRadius(kernelRadius) {}

  size_t GetKernelRadius() const override { return m_kernelRadius; }

  void Process(size_t tileSize, size_t tileOffset, std::vector<ValueType> const & srcValues,
               std::vector<ValueType> & dstValues, ValueType invalidValue) const override
  {
    ProcessMedian(m_kernelRadius, tileSize, tileOffset, srcValues, dstValues, invalidValue);
  }

private:
  size_t m_kernelRadius;
};

template <typename ValueType>
class GaussianFilter : public FilterInterface<ValueType>
{
public:
  GaussianFilter(double standardDeviation, double radiusFactor)
    : m_kernel(CalculateGaussianLinearKernel(standardDeviation, radiusFactor))
  {}

  size_t GetKernelRadius() const override { return m_kernel.size() / 2; }

  void Process(size_t tileSize, size_t tileOffset, std::vector<ValueType> const & srcValues,
               std::vector<ValueType> & dstValues, ValueType invalidValue) const override
  {
    ProcessWithLinearKernel(m_kernel, tileSize, tileOffset, srcValues, dstValues, invalidValue);
  }

private:
  std::vector<double> m_kernel;
};

template <typename ValueType>
using FiltersSequence = std::vector<std::unique_ptr<FilterInterface<ValueType>>>;

template <typename ValueType>
std::vector<ValueType> FilterTile(FiltersSequence<ValueType> const & filters, ms::LatLon const & leftBottom,
                                  size_t stepsInDegree, size_t tileSize, ValuesProvider<ValueType> & valuesProvider)
{
  size_t combinedOffset = 0;
  for (auto const & filter : filters)
    combinedOffset += filter->GetKernelRadius();

  std::vector<ValueType> extTileValues;
  GetExtendedTile(leftBottom, stepsInDegree, tileSize, combinedOffset, valuesProvider, extTileValues);

  std::vector<ValueType> extTileValues2(extTileValues.size());

  size_t const extTileSize = tileSize + 2 * combinedOffset;
  CHECK_EQUAL(extTileSize * extTileSize, extTileValues.size(), ());

  size_t currentOffset = 0;
  for (auto const & filter : filters)
  {
    currentOffset += filter->GetKernelRadius();
    filter->Process(extTileSize, currentOffset, extTileValues, extTileValues2, kInvalidAltitude);
    extTileValues.swap(extTileValues2);
  }

  std::vector<ValueType> result(tileSize * tileSize);
  for (size_t i = combinedOffset; i < extTileSize - combinedOffset; ++i)
  {
    for (size_t j = combinedOffset; j < extTileSize - combinedOffset; ++j)
    {
      size_t const dstIndex = (i - combinedOffset) * tileSize + j - combinedOffset;
      result[dstIndex] = extTileValues[i * extTileSize + j];
    }
  }
  return result;
}
}  // namespace topography_generator
