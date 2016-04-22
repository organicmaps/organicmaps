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
