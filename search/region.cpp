#include "search/region.hpp"

#include "base/assert.hpp"

namespace search
{
bool Region::operator<(Region const & rhs) const
{
  return (m_matchedTokens.size() < rhs.m_matchedTokens.size());
}

bool Region::IsValid() const
{
  if (m_ids.empty())
    return false;
  ASSERT(!m_matchedTokens.empty(), ());
  ASSERT(!m_enName.empty(), ());
  return true;
}

void Region::Swap(Region & rhs)
{
  m_ids.swap(rhs.m_ids);
  m_matchedTokens.swap(rhs.m_matchedTokens);
  m_enName.swap(rhs.m_enName);
}

string DebugPrint(Region const & r)
{
  string res("Region: ");
  res += "Name English: " + r.m_enName;
  res += "; Matched: " + ::DebugPrint(r.m_matchedTokens.size());
  return res;
}
}  // namespace search
