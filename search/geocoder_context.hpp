#pragma once

#include "coding/compressed_bit_vector.hpp"

#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

namespace search
{
class FeaturesFilter;

struct BaseContext
{
  // Advances |curToken| to the nearest unused token, or to the end of
  // |m_usedTokens| if there are no unused tokens.
  size_t SkipUsedTokens(size_t curToken) const;

  // Returns true iff all tokens are used.
  bool AllTokensUsed() const;

  // Returns true if there exists at least one used token in [from,
  // to).
  bool HasUsedTokensInRange(size_t from, size_t to) const;

  // Counts number of groups of consecutive unused tokens.
  size_t NumUnusedTokenGroups() const;

  vector<unique_ptr<coding::CompressedBitVector>> m_features;
  unique_ptr<coding::CompressedBitVector> m_villages;
  coding::CompressedBitVector const * m_streets = nullptr;

  // This vector is used to indicate what tokens were matched by
  // locality and can't be re-used during the geocoding process.
  vector<bool> m_usedTokens;

  size_t m_numTokens = 0;
};
}  // namespace search
