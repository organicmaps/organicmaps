#pragma once

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <sstream>
#include <string>
#include <utility>

#include <boost/optional.hpp>

namespace search
{
template <typename Token>
struct TokenWeightPair
{
  TokenWeightPair() = default;

  template <typename T>
  TokenWeightPair(T && token, double weight) : m_token(std::forward<T>(token)), m_weight(weight)
  {
  }

  bool operator<(TokenWeightPair const & rhs) const
  {
    if (m_token != rhs.m_token)
      return m_token < rhs.m_token;
    return m_weight < rhs.m_weight;
  }

  void Swap(TokenWeightPair & rhs)
  {
    m_token.swap(rhs.m_token);
    std::swap(m_weight, rhs.m_weight);
  }

  // Returns squared weight of the token-weight pair.
  double Sqr() const { return m_weight * m_weight; }

  Token m_token;
  double m_weight = 0;
};

template <typename Token>
std::string DebugPrint(TokenWeightPair<Token> const & tw)
{
  std::ostringstream os;
  os << "TokenWeightPair [ " << DebugPrint(tw.m_token) << ", " << tw.m_weight << " ]";
  return os.str();
}

namespace impl
{
// Accumulates weights of equal tokens in |tws|. Result is sorted by tokens.
template <typename Token>
void SortAndMerge(std::vector<TokenWeightPair<Token>> & tws)
{
  std::sort(tws.begin(), tws.end());
  size_t n = 0;
  for (size_t i = 0; i < tws.size(); ++i)
  {
    ASSERT_LESS_OR_EQUAL(n, i, ());
    if (n == 0 || tws[n - 1].m_token != tws[i].m_token)
    {
      tws[n].Swap(tws[i]);
      ++n;
    }
    else
    {
      tws[n - 1].m_weight += tws[i].m_weight;
    }
  }

  ASSERT_LESS_OR_EQUAL(n, tws.size(), ());
  tws.erase(tws.begin() + n, tws.end());
}

// Computes squared L2 norm of vector of tokens.
template <typename Token>
double SqrL2(std::vector<TokenWeightPair<Token>> const & tws)
{
  double sum = 0;
  for (auto const & tw : tws)
    sum += tw.Sqr();
  return sum;
}

// Computes squared L2 norm of vector of tokens + prefix token.
template <typename Token>
double SqrL2(std::vector<TokenWeightPair<Token>> const & tws,
             boost::optional<TokenWeightPair<Token>> const & prefix)
{
  double result = SqrL2(tws);
  return result + (prefix ? prefix->Sqr() : 0);
}
}  // namespace impl

// This class represents a document in a vector space of tokens.
template <typename Token>
class DocVec
{
public:
  using TokenWeightPair = TokenWeightPair<Token>;
  using TokenWeightPairs = std::vector<TokenWeightPair>;

  class Builder
  {
  public:
    template <typename T>
    void Add(T && token, double weight)
    {
      m_tws.emplace_back(std::forward<T>(token), weight);
    }

  private:
    friend class DocVec;

    std::vector<TokenWeightPair> m_tws;
  };

  DocVec() = default;
  explicit DocVec(Builder && builder) : m_tws(std::move(builder.m_tws)) { Init(); }
  explicit DocVec(Builder const & builder) : m_tws(builder.m_tws) { Init(); }

  TokenWeightPairs const & GetTokenWeightPairs() const { return m_tws; }

  bool Empty() const { return m_tws.empty(); }

private:
  template <typename T>
  friend std::string DebugPrint(DocVec<T> const & dv)
  {
    return "DocVec " + DebugPrint(dv.m_tws);
  }

  void Init() { impl::SortAndMerge(m_tws); }

  TokenWeightPairs m_tws;
};

// This class represents a search query in a vector space of tokens.
template <typename Token>
class QueryVec
{
public:
  using TokenWeightPair = TokenWeightPair<Token>;

  class Builder
  {
  public:
    template <typename T>
    void AddFull(T && token, double weight)
    {
      m_tws.emplace_back(std::forward<T>(token), weight);
    }

    template <typename T>
    void SetPrefix(T && token, double weight)
    {
      m_prefix = TokenWeightPair(std::forward<T>(token), weight);
    }

  private:
    friend class QueryVec;

    std::vector<TokenWeightPair> m_tws;
    boost::optional<TokenWeightPair> m_prefix;
  };

  QueryVec() = default;

  explicit QueryVec(Builder && builder)
    : m_tws(std::move(builder.m_tws)), m_prefix(std::move(builder.m_prefix))
  {
    Init();
  }

  explicit QueryVec(Builder const & builder) : m_tws(builder.m_tws), m_prefix(builder.m_prefix)
  {
    Init();
  }

  // Computes cosine distance between |*this| and |rhs|.
  double Similarity(DocVec<Token> const & rhs) const
  {
    size_t kInvalidIndex = std::numeric_limits<size_t>::max();

    if (Empty() && rhs.Empty())
      return 1.0;

    if (Empty() || rhs.Empty())
      return 0.0;

    auto const & ls = m_tws;
    auto const & rs = rhs.GetTokenWeightPairs();

    ASSERT(std::is_sorted(ls.begin(), ls.end()), ());
    ASSERT(std::is_sorted(rs.begin(), rs.end()), ());

    std::vector<size_t> rsMatchTo(rs.size(), kInvalidIndex);

    size_t i = 0, j = 0;
    double dot = 0;
    while (i < ls.size() && j < rs.size())
    {
      if (ls[i].m_token < rs[j].m_token)
      {
        ++i;
      }
      else if (ls[i].m_token > rs[j].m_token)
      {
        ++j;
      }
      else
      {
        dot += ls[i].m_weight * rs[j].m_weight;
        rsMatchTo[j] = i;
        ++i;
        ++j;
      }
    }

    auto const ln = impl::SqrL2(ls, m_prefix);
    auto const rn = impl::SqrL2(rs);

    // This similarity metric assumes that prefix is not matched in the document.
    double const similarityNoPrefix = ln > 0 && rn > 0 ? dot / sqrt(ln) / sqrt(rn) : 0;

    if (!m_prefix)
      return similarityNoPrefix;

    double similarityWithPrefix = 0;

    auto const & prefix = *m_prefix;

    // Let's try to match prefix token with all tokens in the
    // document, and compute the best cosine distance.
    for (size_t j = 0; j < rs.size(); ++j)
    {
      auto const & tw = rs[j];
      if (!strings::StartsWith(tw.m_token.begin(), tw.m_token.end(), prefix.m_token.begin(),
                               prefix.m_token.end()))
      {
        continue;
      }

      auto const i = rsMatchTo[j];

      double nom = 0;
      double denom = 0;
      if (i == kInvalidIndex)
      {
        // If this document token is not matched with full tokens in a
        // query, we need to update it's weight in the cosine distance
        // - so we need to update correspondingly dot product and
        // vector norms of query and doc.

        // This is the hacky moment: weight of query prefix token may
        // be greater than the weight of the corresponding document
        // token, because the weight of the document token may be
        // unknown at the moment, and be set to some default value.
        // But this heuristic works nicely in practice.
        double const w = std::max(prefix.m_weight, tw.m_weight);
        auto const sqrW = w * w;
        double const l = std::max(0.0, ln - prefix.Sqr() + sqrW);
        double const r = std::max(0.0, rn - tw.Sqr() + sqrW);

        nom = dot + sqrW;
        denom = sqrt(l) * sqrt(r);
      }
      else
      {
        // If this document token is already matched with |i|-th full
        // token in a query - we here that completion of the prefix
        // token is the |i|-th token. So we need to update
        // correspondingly dot product and vector norm of the query.
        double const l = ln + 2 * ls[i].m_weight * prefix.m_weight;

        nom = dot + prefix.m_weight * tw.m_weight;
        denom = sqrt(l) * sqrt(rn);
      }

      if (denom > 0)
        similarityWithPrefix = std::max(similarityWithPrefix, nom / denom);
    }

    return std::max(similarityWithPrefix, similarityNoPrefix);
  }

  double Norm() const
  {
    double n = 0;
    for (auto const & tw : m_tws)
      n += tw.m_weight * tw.m_weight;
    if (m_prefix)
      n += m_prefix->m_weight * m_prefix->m_weight;
    return sqrt(n);
  }

  bool Empty() const { return m_tws.empty() && !m_prefix; }

private:
  template <typename T>
  friend std::string DebugPrint(QueryVec<T> const & qv)
  {
    return "QueryVec " + DebugPrint(qv.m_tws);
  }

  void Init() { impl::SortAndMerge(m_tws); }

  std::vector<TokenWeightPair> m_tws;
  boost::optional<TokenWeightPair> m_prefix;
};
}  // namespace search
