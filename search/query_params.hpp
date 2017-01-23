#pragma once

#include "indexer/scales.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include "std/cstdint.hpp"
#include "std/unordered_set.hpp"
#include "std/vector.hpp"

namespace search
{
class QueryParams
{
public:
  using String = strings::UniString;
  using Synonyms = vector<String>;
  using TypeIndices = vector<uint32_t>;
  using Langs = unordered_set<int8_t>;

  QueryParams() = default;

  template <typename It>
  void Init(It tokenBegin, It tokenEnd)
  {
    Clear();
    for (; tokenBegin != tokenEnd; ++tokenBegin)
      m_tokens.push_back({*tokenBegin});
    m_typeIndices.resize(GetNumTokens());
  }

  template <typename It>
  void InitWithPrefix(It tokenBegin, It tokenEnd, String const & prefix)
  {
    Clear();
    for (; tokenBegin != tokenEnd; ++tokenBegin)
      m_tokens.push_back({*tokenBegin});
    m_prefixTokens.push_back(prefix);
    m_typeIndices.resize(GetNumTokens());
  }

  inline size_t GetNumTokens() const
  {
    return m_prefixTokens.empty() ? m_tokens.size() : m_tokens.size() + 1;
  }

  inline bool LastTokenIsPrefix() const { return !m_prefixTokens.empty(); }

  inline bool IsEmpty() const { return GetNumTokens() == 0; }
  void Clear();

  bool IsCategorySynonym(size_t i) const;
  TypeIndices & GetTypeIndices(size_t i);
  TypeIndices const & GetTypeIndices(size_t i) const;

  bool IsPrefixToken(size_t i) const;
  Synonyms const & GetTokens(size_t i) const;
  Synonyms & GetTokens(size_t i);

  // Returns true if all tokens in [start, end) range have integral
  // synonyms.
  bool IsNumberTokens(size_t start, size_t end) const;

  void RemoveToken(size_t i);

  inline Langs & GetLangs() { return m_langs; }
  inline Langs const & GetLangs() const { return m_langs; }
  inline bool IsLangExist(int8_t lang) const { return m_langs.count(lang) != 0; }

  inline int GetScale() const { return m_scale; }

private:
  friend string DebugPrint(search::QueryParams const & params);

  vector<Synonyms> m_tokens;
  Synonyms m_prefixTokens;
  vector<TypeIndices> m_typeIndices;

  Langs m_langs;
  int m_scale = scales::GetUpperScale();
};
}  // namespace search
