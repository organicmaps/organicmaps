#include "search/geocoder_context.hpp"

#include "search/token_range.hpp"

#include "base/assert.hpp"
#include "base/stl_add.hpp"

#include "std/algorithm.hpp"

namespace search
{
size_t BaseContext::SkipUsedTokens(size_t curToken) const
{
  while (curToken != m_usedTokens.size() && m_usedTokens[curToken])
    ++curToken;
  return curToken;
}

bool BaseContext::AllTokensUsed() const
{
  return all_of(m_usedTokens.begin(), m_usedTokens.end(), IdFunctor());
}

bool BaseContext::HasUsedTokensInRange(TokenRange const & range) const
{
  ASSERT(range.IsValid(), (range));
  return any_of(m_usedTokens.begin() + range.m_begin, m_usedTokens.begin() + range.m_end,
                IdFunctor());
}

size_t BaseContext::NumUnusedTokenGroups() const
{
  size_t numGroups = 0;
  for (size_t i = 0; i < m_usedTokens.size(); ++i)
  {
    if (!m_usedTokens[i] && (i == 0 || m_usedTokens[i - 1]))
      ++numGroups;
  }
  return numGroups;
}
}  // namespace search
