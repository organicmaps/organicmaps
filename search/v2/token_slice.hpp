#pragma once

#include "search/search_query_params.hpp"

#include "base/assert.hpp"

#include "std/cstdint.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace search
{
namespace v2
{
class TokenSlice
{
public:
  TokenSlice(SearchQueryParams const & params, size_t startToken, size_t endToken);

  inline SearchQueryParams::TSynonymsVector const & Get(size_t i) const
  {
    ASSERT_LESS(i, Size(), ());
    return m_params.GetTokens(m_offset + i);
  }

  inline size_t Size() const { return m_size; }

  inline bool Empty() const { return Size() == 0; }

  // Returns true if the |i|-th token in the slice is the incomplete
  // (prefix) token.
  bool IsPrefix(size_t i) const;

  // Returns true if the |i|-th token in the slice is the last
  // (regardless - full or not) token in the query.
  bool IsLast(size_t i) const;

private:
  SearchQueryParams const & m_params;
  size_t const m_offset;
  size_t const m_size;
};

class TokenSliceNoCategories
{
public:
  TokenSliceNoCategories(SearchQueryParams const & params, size_t startToken, size_t endToken);

  inline SearchQueryParams::TSynonymsVector const & Get(size_t i) const
  {
    ASSERT_LESS(i, Size(), ());
    return m_params.GetTokens(m_indexes[i]);
  }

  inline size_t Size() const { return m_indexes.size(); }

  inline bool Empty() const { return Size() == 0; }

  inline bool IsPrefix(size_t i) const
  {
    ASSERT_LESS(i, Size(), ());
    return m_indexes[i] == m_params.m_tokens.size();
  }

private:
  SearchQueryParams const & m_params;
  vector<size_t> m_indexes;
};

string DebugPrint(TokenSlice const & slice);

string DebugPrint(TokenSliceNoCategories const & slice);
}  // namespace v2
}  // namespace search
