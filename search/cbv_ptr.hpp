#pragma once

#include "coding/compressed_bit_vector.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"

#include "std/function.hpp"
#include "std/utility.hpp"

namespace search
{
/// CompressedBitVector pointer class that incapsulates
/// binary operators logic and takes ownership if needed.
class CBVPtr
{
  DISALLOW_COPY(CBVPtr);

  coding::CompressedBitVector const * m_ptr = nullptr;
  bool m_isOwner = false;
  bool m_isFull = false;  ///< True iff all bits are set to one.

  void Release();

public:
  CBVPtr() = default;
  CBVPtr(coding::CompressedBitVector const * p, bool isOwner);
  CBVPtr(CBVPtr && rhs);
  ~CBVPtr();

  void SetFull();
  void Set(coding::CompressedBitVector const * p, bool isOwner = false);
  void Set(unique_ptr<coding::CompressedBitVector> p);

  inline coding::CompressedBitVector const * Get() const { return m_ptr; }

  inline coding::CompressedBitVector const & operator*() const { return *m_ptr; }
  inline coding::CompressedBitVector const * operator->() const { return m_ptr; }
  CBVPtr & operator=(CBVPtr && rhs) noexcept;

  inline bool IsEmpty() const { return !m_isFull && coding::CompressedBitVector::IsEmpty(m_ptr); }
  inline bool IsFull() const { return m_isFull; }
  inline bool IsOwner() const noexcept { return m_isOwner; }

  void Union(coding::CompressedBitVector const * p);
  void Intersect(coding::CompressedBitVector const * p);

  void CopyTo(CBVPtr & rhs);

  template <class TFn>
  void ForEach(TFn && fn) const
  {
    ASSERT(!m_isFull, ());
    if (!IsEmpty())
      coding::CompressedBitVectorEnumerator::ForEach(*m_ptr, forward<TFn>(fn));
  }
};
}  // namespace search
