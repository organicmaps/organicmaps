#pragma once

#include "indexer/scales.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "base/assert.hpp"
#include "base/small_set.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace search
{
class TokenRange;

class QueryParams
{
public:
  using String = strings::UniString;
  using TypeIndices = std::vector<uint32_t>;
  using Langs = base::SafeSmallSet<StringUtf8Multilang::kMaxSupportedLanguages>;

  class Token
  {
  public:
    Token() = default;
    Token(String const & original) : m_original(original) {}

    void AddSynonym(std::string const & s);
    void AddSynonym(String const & s);

    template <typename Fn>
    std::enable_if_t<std::is_same<std::result_of_t<Fn(String)>, void>::value> ForEachSynonym(
        Fn && fn) const
    {
      std::for_each(m_synonyms.begin(), m_synonyms.end(), std::forward<Fn>(fn));
    }

    template <typename Fn>
    std::enable_if_t<std::is_same<std::result_of_t<Fn(String)>, void>::value>
    ForOriginalAndSynonyms(Fn && fn) const
    {
      fn(m_original);
      ForEachSynonym(std::forward<Fn>(fn));
    }

    template <typename Fn>
    std::enable_if_t<std::is_same<std::result_of_t<Fn(String)>, bool>::value, bool> AnyOfSynonyms(
        Fn && fn) const
    {
      return std::any_of(m_synonyms.begin(), m_synonyms.end(), std::forward<Fn>(fn));
    }

    template <typename Fn>
    std::enable_if_t<std::is_same<std::result_of_t<Fn(String)>, bool>::value, bool>
    AnyOfOriginalOrSynonyms(Fn && fn) const
    {
      if (fn(m_original))
        return true;
      return std::any_of(m_synonyms.begin(), m_synonyms.end(), std::forward<Fn>(fn));
    }

    String const & GetOriginal() const { return m_original; }

    void Clear()
    {
      m_original.clear();
      m_synonyms.clear();
    }

  private:
    friend std::string DebugPrint(QueryParams::Token const & token);

    String m_original;
    std::vector<String> m_synonyms;
  };

  QueryParams() = default;

  void SetQuery(std::string const & query) { m_query = query; }

  template <typename It>
  void InitNoPrefix(It tokenBegin, It tokenEnd)
  {
    Clear();
    for (; tokenBegin != tokenEnd; ++tokenBegin)
      m_tokens.emplace_back(*tokenBegin);
    m_typeIndices.resize(GetNumTokens());
    AddSynonyms();
  }

  template <typename It>
  void InitWithPrefix(It tokenBegin, It tokenEnd, String const & prefix)
  {
    Clear();
    for (; tokenBegin != tokenEnd; ++tokenBegin)
      m_tokens.emplace_back(*tokenBegin);
    m_prefixToken = Token(prefix);
    m_hasPrefix = true;
    m_typeIndices.resize(GetNumTokens());
    AddSynonyms();
  }

  size_t GetNumTokens() const { return m_hasPrefix ? m_tokens.size() + 1 : m_tokens.size(); }

  bool LastTokenIsPrefix() const { return m_hasPrefix; }

  bool IsEmpty() const { return GetNumTokens() == 0; }
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

  Langs & GetLangs() { return m_langs; }
  Langs const & GetLangs() const { return m_langs; }
  bool LangExists(int8_t lang) const { return m_langs.Contains(lang); }

  void SetCategorialRequest(bool isCategorial) { m_isCategorialRequest = isCategorial; }
  bool IsCategorialRequest() const { return m_isCategorialRequest; }

private:
  friend std::string DebugPrint(QueryParams const & params);

  void AddSynonyms();

  // The original query without any normalizations.
  std::string m_query;

  std::vector<Token> m_tokens;
  Token m_prefixToken;
  bool m_hasPrefix = false;
  bool m_isCategorialRequest = false;

  std::vector<TypeIndices> m_typeIndices;

  Langs m_langs;
};
}  // namespace search
