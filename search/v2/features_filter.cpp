#include "search/v2/features_filter.hpp"

#include "coding/compressed_bit_vector.hpp"

#include "std/algorithm.hpp"

namespace search
{
namespace v2
{
// FeaturesFilter ----------------------------------------------------------------------------------
FeaturesFilter::FeaturesFilter(coding::CompressedBitVector const & filter, uint32_t threshold)
  : m_filter(filter), m_threshold(threshold)
{
}

bool FeaturesFilter::NeedToFilter(coding::CompressedBitVector const & cbv) const
{
  return cbv.PopCount() > m_threshold;
}

// LocalityFilter ----------------------------------------------------------------------------------
LocalityFilter::LocalityFilter(coding::CompressedBitVector const & filter)
  : FeaturesFilter(filter, 0 /* threshold */)
{
}

unique_ptr<coding::CompressedBitVector> LocalityFilter::Filter(
    coding::CompressedBitVector const & cbv) const
{
  return coding::CompressedBitVector::Intersect(m_filter, cbv);
}

// ViewportFilter ----------------------------------------------------------------------------------
ViewportFilter::ViewportFilter(coding::CompressedBitVector const & filter, uint32_t threshold)
  : FeaturesFilter(filter, threshold)
{
}

unique_ptr<coding::CompressedBitVector> ViewportFilter::Filter(
    coding::CompressedBitVector const & cbv) const
{
  auto result = coding::CompressedBitVector::Intersect(m_filter, cbv);
  if (!coding::CompressedBitVector::IsEmpty(result))
    return result;
  return cbv.LeaveFirstSetNBits(m_threshold);
}
}  // namespace v2
}  // namespace search
