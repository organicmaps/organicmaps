#include "search/token_slice.hpp"

#include "std/sstream.hpp"

namespace search
{
namespace
{
template <typename TSlice>
string SliceToString(string const & name, TSlice const & slice)
{
  ostringstream os;
  os << name << " [";
  for (size_t i = 0; i < slice.Size(); ++i)
  {
    os << DebugPrint(slice.Get(i));
    if (i + 1 != slice.Size())
      os << ", ";
  }
  os << "]";
  return os.str();
}
}  // namespace

TokenSlice::TokenSlice(QueryParams const & params, size_t startToken, size_t endToken)
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

TokenSliceNoCategories::TokenSliceNoCategories(QueryParams const & params, size_t startToken,
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

string DebugPrint(TokenSlice const & slice) { return SliceToString("TokenSlice", slice); }

string DebugPrint(TokenSliceNoCategories const & slice)
{
  return SliceToString("TokenSliceNoCategories", slice);
}

}  // namespace search
