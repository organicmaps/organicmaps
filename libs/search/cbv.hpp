#pragma once

#include "coding/compressed_bit_vector.hpp"

#include "base/ref_counted.hpp"

#include <cstdint>
#include <memory>
#include <utility>

namespace search
{
// A wrapper around coding::CompressedBitVector that augments the
// latter with the "full" state and uses reference counting for
// ownership sharing.
class CBV
{
public:
  static CBV const & GetFull();

  CBV() = default;
  explicit CBV(std::unique_ptr<coding::CompressedBitVector> p);
  CBV(CBV const & cbv) = default;
  CBV(CBV && cbv);

  inline operator bool() const { return !IsEmpty(); }
  CBV & operator=(std::unique_ptr<coding::CompressedBitVector> p);
  CBV & operator=(CBV const & rhs) = default;
  CBV & operator=(CBV && rhs);

  void SetFull();
  void Reset();

  inline bool IsEmpty() const { return !m_isFull && coding::CompressedBitVector::IsEmpty(m_p.Get()); }
  inline bool IsFull() const { return m_isFull; }

  bool HasBit(uint64_t id) const;
  uint64_t PopCount() const;

  template <typename Fn>
  void ForEach(Fn && fn) const
  {
    ASSERT(!m_isFull, ());
    if (!IsEmpty())
      coding::CompressedBitVectorEnumerator::ForEach(*m_p, std::forward<Fn>(fn));
  }

  CBV Union(CBV const & rhs) const;
  CBV Intersect(CBV const & rhs) const;

  // Takes first set |n| bits.
  CBV Take(uint64_t n) const;

  uint64_t Hash() const;

private:
  explicit CBV(bool full);

  base::RefCountPtr<coding::CompressedBitVector> m_p;

  // True iff all bits are set to one.
  bool m_isFull = false;
};
}  // namespace search
