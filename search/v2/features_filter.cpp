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

void CBVPtr::Free()
{
  if (m_isOwner)
    delete m_ptr;

  m_ptr = nullptr;
  m_isOwner = false;
  m_isFull = false;
}

void CBVPtr::Union(coding::CompressedBitVector const * p)
{
  if (!p || m_isFull)
    return;

  if (!m_ptr)
  {
    m_ptr = p;
    m_isFull = false;
  }
  else
    Set(coding::CompressedBitVector::Union(*m_ptr, *p).release(), true);
}

void CBVPtr::Intersect(coding::CompressedBitVector const * p)
{
  if (!p)
  {
    Free();
    return;
  }

  if (m_ptr)
    Set(coding::CompressedBitVector::Intersect(*m_ptr, *p).release(), true);
  else if (m_isFull)
  {
    m_ptr = p;
    m_isFull = false;
  }
}

bool CBVPtr::IsEmpty() const
{
  return !m_isFull && coding::CompressedBitVector::IsEmpty(m_ptr);
}

}  // namespace v2
}  // namespace search
