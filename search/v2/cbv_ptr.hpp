#pragma once

#include "base/macros.hpp"

namespace coding
{
class CompressedBitVector;
}

namespace search
{
namespace v2
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
  ~CBVPtr() { Release(); }

  inline void SetFull()
  {
    Release();
    m_isFull = true;
  }

  void Set(coding::CompressedBitVector const * p, bool isOwner = false);

  inline coding::CompressedBitVector const * Get() const { return m_ptr; }

  bool IsEmpty() const;

  void Union(coding::CompressedBitVector const * p);
  void Intersect(coding::CompressedBitVector const * p);
};

}  // namespace v2
}  // namespace search
