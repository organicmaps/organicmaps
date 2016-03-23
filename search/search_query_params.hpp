#pragma once

#include "base/string_utils.hpp"

#include "std/cstdint.hpp"
#include "std/unordered_set.hpp"
#include "std/vector.hpp"

namespace search
{
struct SearchQueryParams
{
  using TString = strings::UniString;
  using TSynonymsVector = vector<TString>;
  using TLangsSet = unordered_set<int8_t>;

  vector<TSynonymsVector> m_tokens;
  TSynonymsVector m_prefixTokens;
  vector<bool> m_isCategorySynonym;

  TLangsSet m_langs;
  int m_scale;

  SearchQueryParams();

  void Clear();

  /// @param[in] eraseInds Sorted vector of token's indexes.
  void EraseTokens(vector<size_t> & eraseInds);

  void ProcessAddressTokens();

  inline bool IsEmpty() const { return (m_tokens.empty() && m_prefixTokens.empty()); }
  inline bool CanSuggest() const { return (m_tokens.empty() && !m_prefixTokens.empty()); }
  inline bool IsLangExist(int8_t l) const { return (m_langs.count(l) > 0); }

  TSynonymsVector const & GetTokens(size_t i) const;
  TSynonymsVector & GetTokens(size_t i);

  /// @return true if all tokens in [start, end) range has number synonym.
  bool IsNumberTokens(size_t start, size_t end) const;

private:
  template <class ToDo>
  void ForEachToken(ToDo && toDo);
};

string DebugPrint(search::SearchQueryParams const & params);
}  // namespace search
