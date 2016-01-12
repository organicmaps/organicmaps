#include "search/v2/features_filter.hpp"

#include "coding/compressed_bit_vector.hpp"

namespace search
{
namespace v2
{
FeaturesFilter::FeaturesFilter() : m_filter(nullptr), m_threshold(0) {}

FeaturesFilter::FeaturesFilter(coding::CompressedBitVector const & filter, uint32_t threshold)
  : m_filter(&filter), m_threshold(threshold)
{
}

bool FeaturesFilter::NeedToFilter(coding::CompressedBitVector const & cbv) const
{
  return cbv.PopCount() > m_threshold;
}

unique_ptr<coding::CompressedBitVector> FeaturesFilter::Filter(
    coding::CompressedBitVector const & cbv) const
{
  if (!m_filter)
    return make_unique<coding::SparseCBV>();
  return coding::CompressedBitVector::Intersect(*m_filter, cbv);
}

}  // namespace v2
}  // namespace search
