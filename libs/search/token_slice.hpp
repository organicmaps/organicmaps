#pragma once

#include "search/query_params.hpp"
#include "search/token_range.hpp"

#include "indexer/string_slice.hpp"

#include "base/assert.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace search
{
class TokenSlice
{
public:
  TokenSlice(QueryParams const & params, TokenRange const & range);

  inline QueryParams::Token const & Get(size_t i) const
  {
    ASSERT_LESS(i, Size(), ());
    return m_params.GetToken(m_offset + i);
  }

  inline size_t Size() const { return m_size; }

  inline bool Empty() const { return Size() == 0; }

  // Returns true if the |i|-th token in the slice is the incomplete
  // (prefix) token.
  bool IsPrefix(size_t i) const;

private:
  QueryParams const & m_params;
  size_t const m_offset;
  size_t const m_size;
};

class TokenSliceNoCategories
{
public:
  TokenSliceNoCategories(QueryParams const & params, TokenRange const & range);

  inline QueryParams::Token const & Get(size_t i) const
  {
    ASSERT_LESS(i, Size(), ());
    return m_params.GetToken(m_indexes[i]);
  }

  inline size_t Size() const { return m_indexes.size(); }

  inline bool Empty() const { return Size() == 0; }
  inline bool IsPrefix(size_t i) const { return m_params.IsPrefixToken(m_indexes[i]); }

private:
  QueryParams const & m_params;
  std::vector<size_t> m_indexes;
};

class QuerySlice : public StringSliceBase
{
public:
  explicit QuerySlice(TokenSlice const & slice) : m_slice(slice) {}

  // QuerySlice overrides:
  QueryParams::String const & Get(size_t i) const override { return m_slice.Get(i).GetOriginal(); }
  size_t Size() const override { return m_slice.Size(); }

private:
  TokenSlice const & m_slice;
};

template <typename Cont>
class QuerySliceOnRawStrings : public StringSliceBase
{
public:
  QuerySliceOnRawStrings(Cont const & tokens, String const & prefix) : m_tokens(tokens), m_prefix(prefix) {}

  bool HasPrefixToken() const { return !m_prefix.empty(); }

  // QuerySlice overrides:
  QueryParams::String const & Get(size_t i) const override
  {
    ASSERT_LESS(i, Size(), ());
    return i == m_tokens.size() ? m_prefix : m_tokens[i];
  }

  size_t Size() const override { return m_tokens.size() + (m_prefix.empty() ? 0 : 1); }

private:
  Cont const & m_tokens;
  QueryParams::String const & m_prefix;
};

std::string DebugPrint(TokenSlice const & slice);

std::string DebugPrint(TokenSliceNoCategories const & slice);
}  // namespace search
