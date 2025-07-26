#include "search/cbv.hpp"

#include <limits>
#include <vector>

using namespace std;

namespace search
{
namespace
{
uint64_t const kModulo = 18446744073709551557LLU;
}  // namespace

// static
CBV const & CBV::GetFull()
{
  static CBV const fullCBV(true /*full*/);
  return fullCBV;
}

CBV::CBV(unique_ptr<coding::CompressedBitVector> p) : m_p(std::move(p)) {}

CBV::CBV(CBV && cbv) : m_p(std::move(cbv.m_p)), m_isFull(cbv.m_isFull)
{
  cbv.m_isFull = false;
}

CBV::CBV(bool full) : m_isFull(full) {}

CBV & CBV::operator=(unique_ptr<coding::CompressedBitVector> p)
{
  m_p = std::move(p);
  m_isFull = false;

  return *this;
}

CBV & CBV::operator=(CBV && rhs)
{
  if (this == &rhs)
    return *this;

  m_p = std::move(rhs.m_p);
  m_isFull = rhs.m_isFull;

  rhs.m_isFull = false;

  return *this;
}

void CBV::SetFull()
{
  m_p.Reset();
  m_isFull = true;
}

void CBV::Reset()
{
  m_p.Reset();
  m_isFull = false;
}

bool CBV::HasBit(uint64_t id) const
{
  if (IsFull())
    return true;
  if (IsEmpty())
    return false;
  return m_p->GetBit(id);
}

uint64_t CBV::PopCount() const
{
  ASSERT(!IsFull(), ());
  if (IsEmpty())
    return 0;
  return m_p->PopCount();
}

CBV CBV::Union(CBV const & rhs) const
{
  if (IsFull() || rhs.IsEmpty())
    return *this;
  if (IsEmpty() || rhs.IsFull())
    return rhs;
  return CBV(coding::CompressedBitVector::Union(*m_p, *rhs.m_p));
}

CBV CBV::Intersect(CBV const & rhs) const
{
  if (IsFull() || rhs.IsEmpty())
    return rhs;
  if (IsEmpty() || rhs.IsFull())
    return *this;
  return CBV(coding::CompressedBitVector::Intersect(*m_p, *rhs.m_p));
}

CBV CBV::Take(uint64_t n) const
{
  if (IsEmpty())
    return *this;
  if (IsFull())
  {
    vector<uint64_t> groups(static_cast<size_t>((n + 63) / 64), numeric_limits<uint64_t>::max());
    uint64_t const r = n % 64;
    if (r != 0)
    {
      ASSERT(!groups.empty(), ());
      groups.back() = (static_cast<uint64_t>(1) << r) - 1;
    }
    return CBV(coding::DenseCBV::BuildFromBitGroups(std::move(groups)));
  }

  return CBV(m_p->LeaveFirstSetNBits(n));
}

uint64_t CBV::Hash() const
{
  if (IsEmpty())
    return 0;
  if (IsFull())
    return kModulo;
  return coding::CompressedBitVectorHasher::Hash(*m_p) % kModulo;
}
}  // namespace search
