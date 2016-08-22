#pragma once

#include "indexer/scales.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include "std/cstdint.hpp"
#include "std/unordered_set.hpp"
#include "std/vector.hpp"

namespace search
{
struct QueryParams
{
  using TString = strings::UniString;
  using TSynonymsVector = vector<TString>;
  using TLangsSet = unordered_set<int8_t>;

  QueryParams() = default;

  inline size_t GetNumTokens() const
  {
    return m_prefixTokens.empty() ? m_tokens.size() : m_tokens.size() + 1;
  }

  inline bool IsEmpty() const { return GetNumTokens() == 0; }
  inline bool IsLangExist(int8_t lang) const { return m_langs.count(lang) != 0; }

  void Clear();

  bool IsCategorySynonym(size_t i) const;
  bool IsPrefixToken(size_t i) const;
  TSynonymsVector const & GetTokens(size_t i) const;
  TSynonymsVector & GetTokens(size_t i);

  // Returns true if all tokens in [start, end) range has intergral
  // synonyms.
  bool IsNumberTokens(size_t start, size_t end) const;

  vector<TSynonymsVector> m_tokens;
  TSynonymsVector m_prefixTokens;
  vector<vector<uint32_t>> m_types;

  TLangsSet m_langs;
  int m_scale = scales::GetUpperScale();
};

string DebugPrint(search::QueryParams const & params);
}  // namespace search
