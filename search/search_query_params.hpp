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

  /// @param[in] eraseInds Sorted vector of token's indexes.
  void EraseTokens(vector<size_t> & eraseInds);

  void ProcessAddressTokens();

  inline bool IsEmpty() const { return (m_tokens.empty() && m_prefixTokens.empty()); }
  inline bool CanSuggest() const { return (m_tokens.empty() && !m_prefixTokens.empty()); }
  inline bool IsLangExist(int8_t l) const { return (m_langs.count(l) > 0); }

  vector<TSynonymsVector> m_tokens;
  TSynonymsVector m_prefixTokens;
  TLangsSet m_langs;

private:
  template <class ToDo>
  void ForEachToken(ToDo && toDo);
};
}  // namespace search

string DebugPrint(search::SearchQueryParams const & params);
