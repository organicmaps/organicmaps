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
  DISALLOW_COPY_AND_MOVE(CBVPtr);

  coding::CompressedBitVector const * m_ptr = nullptr;
  bool m_isOwner = false;
  bool m_isFull = false;  ///< True iff all bits are set to one.

  void Release();

public:
  CBVPtr() = default;
  CBVPtr(coding::CompressedBitVector const * p, bool isOwner);
  ~CBVPtr() { Release(); }

  inline void SetFull()
  {
    Release();
    m_isFull = true;
  }

  void Set(coding::CompressedBitVector const * p, bool isOwner = false);
  void Set(unique_ptr<coding::CompressedBitVector> p);

  inline coding::CompressedBitVector const * Get() const { return m_ptr; }

  coding::CompressedBitVector const & operator*() const { return *m_ptr; }
  coding::CompressedBitVector const * operator->() const { return m_ptr; }

  bool IsEmpty() const;

  void Union(coding::CompressedBitVector const * p);
  void Intersect(coding::CompressedBitVector const * p);

  template <class TFn>
  void ForEach(TFn && fn) const
  {
    ASSERT(!m_isFull, ());
    if (!IsEmpty())
      coding::CompressedBitVectorEnumerator::ForEach(*m_ptr, forward<TFn>(fn));
  }
};

}  // namespace search
