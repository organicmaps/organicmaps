#include "search/v2/token_slice.hpp"

namespace search
{
namespace v2
{
TokenSlice::TokenSlice(SearchQueryParams const & params, size_t startToken, size_t endToken)
  : m_params(params), m_offset(startToken), m_size(endToken - startToken)
{
  ASSERT_LESS_OR_EQUAL(startToken, endToken, ());
}

bool TokenSlice::IsPrefix(size_t i) const
{
  ASSERT_LESS(i, Size(), ());
  return m_offset + i == m_params.m_tokens.size();
}

bool TokenSlice::IsLast(size_t i) const
{
  ASSERT_LESS(i, Size(), ());
  if (m_params.m_prefixTokens.empty())
    return m_offset + i + 1 == m_params.m_tokens.size();
  return m_offset + i == m_params.m_tokens.size();
}

TokenSliceNoCategories::TokenSliceNoCategories(SearchQueryParams const & params, size_t startToken,
                                               size_t endToken)
  : m_params(params)
{
  ASSERT_LESS_OR_EQUAL(startToken, endToken, ());

  m_indexes.reserve(endToken - startToken);
  for (size_t i = startToken; i < endToken; ++i)
  {
    if (!m_params.m_isCategorySynonym[i])
      m_indexes.push_back(i);
  }
}
}  // namespace v2
}  // namespace search
