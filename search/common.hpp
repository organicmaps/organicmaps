#pragma once

#include "indexer/categories_holder.hpp"

#include "base/buffer_vector.hpp"
#include "base/small_set.hpp"
#include "base/string_utils.hpp"

namespace search
{
// The prefix is stored separately.
// todo(@m, @y) Find a way (similar to TokenSlice maybe?) to unify
//              the prefix and non-prefix tokens.
using QueryTokens = buffer_vector<strings::UniString, 32>;

using Locales = base::SafeSmallSet<CategoriesHolder::kLocaleMapping.size() + 1>;

/// Upper bound for max count of tokens for indexing and scoring.
size_t constexpr kMaxNumTokens = 32;
size_t constexpr kMaxNumSuggests = 5;

struct QueryString
{
  std::string m_query;          ///< raw UTF8 query string
  QueryTokens m_tokens;         ///< splitted by UniChar tokens (not including last prefix)
  strings::UniString m_prefix;  ///< last prefix or empty (if query is ended with separator)

  bool IsEmpty() const { return m_tokens.empty() && m_prefix.empty(); }
};

}  // namespace search
