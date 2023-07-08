#pragma once

#include "indexer/categories_holder.hpp"

#include "base/buffer_vector.hpp"
#include "base/small_set.hpp"
#include "base/string_utils.hpp"

#include <cstddef>

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
}  // namespace search
