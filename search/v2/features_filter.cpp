#include "search/v2/features_filter.hpp"

namespace search
{
namespace v2
{
FeaturesFilter::FeaturesFilter() : m_threshold(0) {}

FeaturesFilter::FeaturesFilter(unique_ptr<coding::CompressedBitVector> filter, uint32_t threshold)
  : m_filter(move(filter)), m_threshold(threshold)
{
}

bool FeaturesFilter::NeedToFilter(coding::CompressedBitVector & cbv) const
{
  return cbv.PopCount() > m_threshold;
}

unique_ptr<coding::CompressedBitVector> FeaturesFilter::Filter(
    coding::CompressedBitVector & cbv) const
{
  if (!m_filter)
    return make_unique<coding::SparseCBV>();
  return coding::CompressedBitVector::Intersect(*m_filter, cbv);
}
}  // namespace v2
}  // namespace search
