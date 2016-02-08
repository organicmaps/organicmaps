#include "search/v2/cbv_ptr.hpp"

#include "coding/compressed_bit_vector.hpp"

namespace search
{
namespace v2
{
CBVPtr::CBVPtr(coding::CompressedBitVector const * p, bool isOwner)
{
  Set(p, isOwner);
}

void CBVPtr::Release()
{
  if (m_isOwner)
    delete m_ptr;

  m_ptr = nullptr;
  m_isOwner = false;
  m_isFull = false;
}

void CBVPtr::Set(coding::CompressedBitVector const * p, bool isOwner/* = false*/)
{
  Release();

  m_ptr = p;
  m_isOwner = p && isOwner;
}

void CBVPtr::Set(unique_ptr<coding::CompressedBitVector> p)
{
  Set(p.release(), true /* isOwner */);
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
  {
    Set(coding::CompressedBitVector::Union(*m_ptr, *p).release(), true);
  }
}

void CBVPtr::Intersect(coding::CompressedBitVector const * p)
{
  if (!p)
  {
    Release();
    return;
  }

  if (m_ptr)
  {
    Set(coding::CompressedBitVector::Intersect(*m_ptr, *p).release(), true);
  }
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
