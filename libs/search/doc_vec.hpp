#pragma once

#include "search/idf_map.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

namespace search
{
class IdfMap;

struct TokenFrequencyPair
{
  TokenFrequencyPair() = default;

  template <typename Token>
  TokenFrequencyPair(Token && token, uint64_t frequency) : m_token(std::forward<Token>(token))
                                                         , m_frequency(frequency)
  {}

  bool operator<(TokenFrequencyPair const & rhs) const;

  void Swap(TokenFrequencyPair & rhs);

  strings::UniString m_token;
  uint64_t m_frequency = 0;
};

std::string DebugPrint(TokenFrequencyPair const & tf);

// This class represents a document in a vector space of tokens.
class DocVec
{
public:
  class Builder
  {
  public:
    template <typename Token>
    void Add(Token && token)
    {
      m_tokens.emplace_back(std::forward<Token>(token));
    }

  private:
    friend class DocVec;

    std::vector<strings::UniString> m_tokens;
  };

  DocVec() = default;
  explicit DocVec(Builder const & builder);

  // Computes vector norm of the doc.
  double Norm(IdfMap & idfs) const;

  size_t GetNumTokens() const { return m_tfs.size(); }

  strings::UniString const & GetToken(size_t i) const;
  double GetIdf(IdfMap & idfs, size_t i) const;
  double GetWeight(IdfMap & idfs, size_t i) const;

  bool Empty() const { return m_tfs.empty(); }

private:
  friend std::string DebugPrint(DocVec const & dv) { return "DocVec " + ::DebugPrint(dv.m_tfs); }

  std::vector<TokenFrequencyPair> m_tfs;
};

// This class represents a search query in a vector space of tokens.
class QueryVec
{
public:
  class Builder
  {
  public:
    template <typename Token>
    void AddFull(Token && token)
    {
      m_tokens.emplace_back(std::forward<Token>(token));
    }

    template <typename Token>
    void SetPrefix(Token && token)
    {
      m_prefix = std::forward<Token>(token);
    }

  private:
    friend class QueryVec;

    std::vector<strings::UniString> m_tokens;
    std::optional<strings::UniString> m_prefix;
  };

  explicit QueryVec(IdfMap & idfs) : m_idfs(&idfs) {}

  QueryVec(IdfMap & idfs, Builder const & builder);

  // Computes cosine similarity between |*this| and |rhs|.
  double Similarity(IdfMap & docIdfs, DocVec const & rhs);

  // Computes vector norm of the query.
  double Norm();

  bool Empty() const { return m_tfs.empty() && !m_prefix; }

private:
  double GetFullTokenWeight(size_t i);
  double GetPrefixTokenWeight();

  friend std::string DebugPrint(QueryVec const & qv)
  {
    std::ostringstream os;
    os << "QueryVec " + ::DebugPrint(qv.m_tfs);
    if (qv.m_prefix)
      os << " " << DebugPrint(*qv.m_prefix);
    return os.str();
  }

  IdfMap * m_idfs;
  std::vector<TokenFrequencyPair> m_tfs;
  std::optional<strings::UniString> m_prefix;
};
}  // namespace search
