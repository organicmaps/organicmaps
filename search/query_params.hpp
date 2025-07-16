#pragma once

#include "coding/string_utf8_multilang.hpp"

#include "base/assert.hpp"
#include "base/small_set.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <string>
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
    void ForEachSynonym(Fn && fn) const
    {
      std::for_each(m_synonyms.begin(), m_synonyms.end(), std::forward<Fn>(fn));
    }

    template <typename Fn>
    void ForOriginalAndSynonyms(Fn && fn) const
    {
      fn(m_original);
      ForEachSynonym(std::forward<Fn>(fn));
    }

    template <typename Fn>
    bool AnyOfSynonyms(Fn && fn) const
    {
      return std::any_of(m_synonyms.begin(), m_synonyms.end(), std::forward<Fn>(fn));
    }

    template <typename Fn>
    bool AnyOfOriginalOrSynonyms(Fn && fn) const
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

  template <typename IterT>
  void Init(std::string const & /*query*/, IterT tokenBegin, IterT tokenEnd, String const & prefix)
  {
    Clear();

    for (; tokenBegin != tokenEnd; ++tokenBegin)
      m_tokens.emplace_back(*tokenBegin);

    if (!prefix.empty())
    {
      m_prefixToken = Token(prefix);
      m_hasPrefix = true;
    }

    m_typeIndices.resize(GetNumTokens());

    AddSynonyms();
  }

  template <typename ContT>
  void Init(std::string const & query, ContT const & tokens, bool isLastPrefix)
  {
    CHECK(!isLastPrefix || !tokens.empty(), ());

    Init(query, tokens.begin(), isLastPrefix ? tokens.end() - 1 : tokens.end(),
         isLastPrefix ? tokens.back() : String{});
  }

  void ClearStreetIndices();

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

  bool IsCommonToken(size_t i) const;

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

  std::vector<Token> m_tokens;
  Token m_prefixToken;
  bool m_hasPrefix = false;
  bool m_isCategorialRequest = false;

  std::vector<bool> m_isCommonToken;

  std::vector<TypeIndices> m_typeIndices;

  Langs m_langs;
};
}  // namespace search
