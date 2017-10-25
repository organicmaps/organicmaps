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

using Locales =
    base::SafeSmallSet<static_cast<uint64_t>(CategoriesHolder::kMaxSupportedLocaleIndex) + 1>;

/// Upper bound for max count of tokens for indexing and scoring.
int constexpr MAX_TOKENS = 32;
int constexpr MAX_SUGGESTS_COUNT = 5;
}  // namespace search
