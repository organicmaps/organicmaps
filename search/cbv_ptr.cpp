#include "search/cbv_ptr.hpp"

namespace search
{
CBVPtr::CBVPtr(coding::CompressedBitVector const * p, bool isOwner) { Set(p, isOwner); }

CBVPtr::CBVPtr(CBVPtr && rhs) { *this = move(rhs); }

CBVPtr::~CBVPtr() { Release(); }

void CBVPtr::Release()
{
  if (m_isOwner)
    delete m_ptr;

  m_ptr = nullptr;
  m_isOwner = false;
  m_isFull = false;
}

void CBVPtr::SetFull()
{
  Release();
  m_isFull = true;
}

void CBVPtr::Set(coding::CompressedBitVector const * p, bool isOwner /* = false*/)
{
  Release();

  m_ptr = p;
  m_isOwner = p && isOwner;
}

void CBVPtr::Set(unique_ptr<coding::CompressedBitVector> p)
{
  Set(p.release(), true /* isOwner */);
}

CBVPtr & CBVPtr::operator=(CBVPtr && rhs) noexcept
{
  if (this == &rhs)
    return *this;

  m_ptr = rhs.m_ptr;
  m_isOwner = rhs.m_isOwner;
  m_isFull = rhs.m_isFull;

  rhs.m_ptr = nullptr;
  rhs.m_isOwner = false;
  rhs.m_isFull = false;

  return *this;
}

void CBVPtr::Union(coding::CompressedBitVector const * p)
{
  if (!p || m_isFull)
    return;

  if (!m_ptr)
  {
    m_ptr = p;
    m_isFull = false;
    m_isOwner = false;
  }
  else
  {
    Set(coding::CompressedBitVector::Union(*m_ptr, *p).release(), true /* isOwner */);
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
    Set(coding::CompressedBitVector::Intersect(*m_ptr, *p).release(), true /* isOwner */);
  }
  else if (m_isFull)
  {
    m_ptr = p;
    m_isFull = false;
    m_isOwner = false;
  }
}

void CBVPtr::CopyTo(CBVPtr & rhs)
{
  rhs.Release();

  if (m_isFull)
  {
    rhs.SetFull();
    return;
  }

  if (m_ptr)
    rhs.Set(m_ptr->Clone());
}
}  // namespace search
