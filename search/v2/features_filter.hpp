#pragma once

#include "base/macros.hpp"

#include "std/unique_ptr.hpp"

namespace coding
{
class CompressedBitVector;
}

namespace search
{
namespace v2
{
// A lightweight filter of features.
//
// NOTE: this class *IS NOT* thread-safe.
class FeaturesFilter
{
public:
  FeaturesFilter();

  FeaturesFilter(coding::CompressedBitVector const & filter, uint32_t threshold);

  inline void SetFilter(coding::CompressedBitVector const * filter) { m_filter = filter; }

  inline void SetThreshold(uint32_t threshold) { m_threshold = threshold; }

  bool NeedToFilter(coding::CompressedBitVector const & features) const;
  unique_ptr<coding::CompressedBitVector> Filter(coding::CompressedBitVector const & cbv) const;

private:
  // Non-owning ptr.
  coding::CompressedBitVector const * m_filter;
  uint32_t m_threshold;
};


/// CompressedBitVector pointer class that incapsulates
/// binary operators logic and takes ownership if needed.
class CBVPtr
{
  DISALLOW_COPY_AND_MOVE(CBVPtr);

  coding::CompressedBitVector const * m_ptr = nullptr;
  bool m_isOwner = false;
  bool m_isFull = false;

  void Free();

public:
  CBVPtr() = default;
  ~CBVPtr() { Free(); }

  inline void SetFull()
  {
    Free();
    m_isFull = true;
  }

  inline void Set(coding::CompressedBitVector const * p, bool isOwner = false)
  {
    Free();

    m_ptr = p;
    m_isOwner = p && isOwner;
  }

  inline coding::CompressedBitVector const * Get() const { return m_ptr; }

  bool IsEmpty() const;

  void Union(coding::CompressedBitVector const * p);
  void Intersect(coding::CompressedBitVector const * p);
};

}  // namespace v2
}  // namespace search
