#pragma once

#include "indexer/scales.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "base/assert.hpp"
#include "base/small_set.hpp"
#include "base/string_utils.hpp"

#include "std/cstdint.hpp"
#include "std/type_traits.hpp"
#include "std/unordered_set.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace search
{
class TokenRange;

class QueryParams
{
public:
  using String = strings::UniString;
  using TypeIndices = vector<uint32_t>;
  using Langs = base::SafeSmallSet<StringUtf8Multilang::kMaxSupportedLanguages>;

  struct Token
  {
    Token() = default;
    Token(String const & original) : m_original(original) {}

    void AddSynonym(String const & s) { m_synonyms.push_back(s); }
    void AddSynonym(string const & s) { m_synonyms.push_back(strings::MakeUniString(s)); }

    // Calls |fn| on the original token and on synonyms.
    template <typename Fn>
    typename enable_if<is_same<typename result_of<Fn(String)>::type, void>::value>::type ForEach(
        Fn && fn) const
    {
      fn(m_original);
      for_each(m_synonyms.begin(), m_synonyms.end(), forward<Fn>(fn));
    }

    // Calls |fn| on the original token and on synonyms until |fn| return false.
    template <typename Fn>
    typename enable_if<is_same<typename result_of<Fn(String)>::type, bool>::value>::type ForEach(
        Fn && fn) const
    {
      if (!fn(m_original))
        return;
      for (auto const & synonym : m_synonyms)
      {
        if (!fn(synonym))
          return;
      }
    }

    void Clear()
    {
      m_original.clear();
      m_synonyms.clear();
    }

    String m_original;
    vector<String> m_synonyms;
  };

  QueryParams() = default;

  template <typename It>
  void InitNoPrefix(It tokenBegin, It tokenEnd)
  {
    Clear();
    for (; tokenBegin != tokenEnd; ++tokenBegin)
      m_tokens.emplace_back(*tokenBegin);
    m_typeIndices.resize(GetNumTokens());
  }

  template <typename It>
  void InitWithPrefix(It tokenBegin, It tokenEnd, String const & prefix)
  {
    Clear();
    for (; tokenBegin != tokenEnd; ++tokenBegin)
      m_tokens.push_back(*tokenBegin);
    m_prefixToken.m_original = prefix;
    m_hasPrefix = true;
    m_typeIndices.resize(GetNumTokens());
  }

  inline size_t GetNumTokens() const
  {
    return m_hasPrefix ? m_tokens.size() + 1: m_tokens.size();
  }

  inline bool LastTokenIsPrefix() const { return m_hasPrefix; }

  inline bool IsEmpty() const { return GetNumTokens() == 0; }
  void Clear();

  bool IsCategorySynonym(size_t i) const;
  TypeIndices & GetTypeIndices(size_t i);
  TypeIndices const & GetTypeIndices(size_t i) const;

  bool IsPrefixToken(size_t i) const;
  Token const & GetToken(size_t i) const;
  Token & GetToken(size_t i);

  // Returns true if all tokens in |range| have integral synonyms.
  bool IsNumberTokens(TokenRange const & range) const;

  void RemoveToken(size_t i);

  inline Langs & GetLangs() { return m_langs; }
  inline Langs const & GetLangs() const { return m_langs; }
  inline bool LangExists(int8_t lang) const { return m_langs.Contains(lang); }

  inline void SetCategorialRequest(bool rhs) { m_isCategorialRequest = rhs; }
  inline bool IsCategorialRequest() const { return m_isCategorialRequest; }

  inline int GetScale() const { return m_scale; }

private:
  friend string DebugPrint(QueryParams const & params);

  vector<Token> m_tokens;
  Token m_prefixToken;
  bool m_hasPrefix = false;
  bool m_isCategorialRequest = false;

  vector<TypeIndices> m_typeIndices;

  Langs m_langs;
  int m_scale = scales::GetUpperScale();
};

string DebugPrint(QueryParams::Token const & token);
}  // namespace search
