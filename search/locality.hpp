#pragma once

#include "indexer/ftypes_matcher.hpp"
#include "indexer/search_trie.hpp"

#include "base/buffer_vector.hpp"
#include "base/string_utils.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

namespace search
{
struct Locality
{
  using TToken = strings::UniString;
  using TTokensArray = buffer_vector<TToken, 32>;

  // Native and English names of locality.
  string m_name;
  string m_enName;

  // Indexes of matched tokens for locality.
  vector<size_t> m_matchedTokens;

  ftypes::Type m_type;
  uint32_t m_featureId;
  m2::PointD m_center;
  uint8_t m_rank;
  double m_radius;

  Locality();

  Locality(ftypes::Type type, uint32_t featureId, m2::PointD const & center, uint8_t rank);

  bool IsValid() const;

  bool IsSuitable(TTokensArray const & tokens, TToken const & prefix) const;

  void Swap(Locality & rhs);

  bool operator<(Locality const & rhs) const;

private:
  bool IsFullNameMatched() const;

  size_t GetSynonymTokenLength(TTokensArray const & tokens, TToken const & prefix) const;
};

string DebugPrint(Locality const & l);
}  // namespace search
