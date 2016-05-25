#pragma once

#include "search/query_params.hpp"

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
  TokenSlice(QueryParams const & params, size_t startToken, size_t endToken);

  inline QueryParams::TSynonymsVector const & Get(size_t i) const
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
  QueryParams const & m_params;
  size_t const m_offset;
  size_t const m_size;
};

class TokenSliceNoCategories
{
public:
  TokenSliceNoCategories(QueryParams const & params, size_t startToken, size_t endToken);

  inline QueryParams::TSynonymsVector const & Get(size_t i) const
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
  QueryParams const & m_params;
  vector<size_t> m_indexes;
};

class QuerySlice
{
public:
  using TString = QueryParams::TString;

  virtual ~QuerySlice() = default;

  virtual TString const & Get(size_t i) const = 0;
  virtual size_t Size() const = 0;
  virtual bool IsPrefix(size_t i) const = 0;

  bool Empty() const { return Size() == 0; }
};

class QuerySliceOnTokens : public QuerySlice
{
public:
  QuerySliceOnTokens(TokenSlice const & slice) : m_slice(slice) {}

  // QuerySlice overrides:
  QueryParams::TString const & Get(size_t i) const override { return m_slice.Get(i).front(); }
  size_t Size() const override { return m_slice.Size(); }
  bool IsPrefix(size_t i) const override { return m_slice.IsPrefix(i); }

private:
  TokenSlice const m_slice;
};

template <typename TCont>
class QuerySliceOnRawStrings : public QuerySlice
{
public:
  QuerySliceOnRawStrings(TCont const & tokens, TString const & prefix)
    : m_tokens(tokens), m_prefix(prefix)
  {
  }

  // QuerySlice overrides:
  QueryParams::TString const & Get(size_t i) const override
  {
    ASSERT_LESS(i, Size(), ());
    return i == m_tokens.size() ? m_prefix : m_tokens[i];
  }

  size_t Size() const override { return m_tokens.size() + (m_prefix.empty() ? 0 : 1); }

  bool IsPrefix(size_t i) const override
  {
    ASSERT_LESS(i, Size(), ());
    return i == m_tokens.size();
  }

 private:
  TCont const & m_tokens;
  TString const & m_prefix;
};

string DebugPrint(TokenSlice const & slice);

string DebugPrint(TokenSliceNoCategories const & slice);
}  // namespace v2
}  // namespace search
