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
  double SqrWeight() const { return m_weight * m_weight; }

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
// Accumulates weights of equal tokens in |tws|. Result is sorted by
// tokens. Also, maximum weight from a group of equal tokens will be
// stored in the corresponding |maxWeight| elem.
template <typename Token>
void SortAndMerge(std::vector<TokenWeightPair<Token>> & tws, std::vector<double> & maxWeights)
{
  std::sort(tws.begin(), tws.end());
  size_t n = 0;
  maxWeights.clear();
  for (size_t i = 0; i < tws.size(); ++i)
  {
    ASSERT_LESS_OR_EQUAL(n, i, ());
    ASSERT_EQUAL(n, maxWeights.size(), ());

    if (n == 0 || tws[n - 1].m_token != tws[i].m_token)
    {
      tws[n].Swap(tws[i]);
      maxWeights.push_back(tws[n].m_weight);
      ++n;
    }
    else
    {
      tws[n - 1].m_weight += tws[i].m_weight;
      maxWeights[n - 1] = std::max(maxWeights[n - 1], tws[i].m_weight);
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
    sum += tw.SqrWeight();
  return sum;
}

// Computes squared L2 norm of vector of tokens + prefix token.
template <typename Token>
double SqrL2(std::vector<TokenWeightPair<Token>> const & tws,
             boost::optional<TokenWeightPair<Token>> const & prefix)
{
  double result = SqrL2(tws);
  return result + (prefix ? prefix->SqrWeight() : 0);
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

    TokenWeightPairs m_tws;
  };

  DocVec() = default;
  explicit DocVec(Builder && builder) : m_tws(std::move(builder.m_tws)) { Init(); }
  explicit DocVec(Builder const & builder) : m_tws(builder.m_tws) { Init(); }

  TokenWeightPairs const & GetTokenWeightPairs() const { return m_tws; }
  std::vector<double> const & GetMaxWeights() const { return m_maxWeights; }

  bool Empty() const { return m_tws.empty(); }

private:
  template <typename T>
  friend std::string DebugPrint(DocVec<T> const & dv)
  {
    return "DocVec " + DebugPrint(dv.m_tws);
  }

  void Init() { impl::SortAndMerge(m_tws, m_maxWeights); }

  TokenWeightPairs m_tws;
  std::vector<double> m_maxWeights;
};

// This class represents a search query in a vector space of tokens.
template <typename Token>
class QueryVec
{
public:
  using TokenWeightPair = TokenWeightPair<Token>;
  using TokenWeightPairs = std::vector<TokenWeightPair>;

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

    TokenWeightPairs m_tws;
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
    auto const & maxWeights = rhs.GetMaxWeights();

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
        auto const w = maxWeights[j];
        auto const l = std::max(0.0, ln - prefix.SqrWeight() + w * w);

        nom = dot + w * tw.m_weight;
        denom = sqrt(l) * sqrt(rn);
      }
      else
      {
        // If this document token is already matched with |i|-th full
        // token in a query - we know that completion of the prefix
        // token is the |i|-th query token. So we need to update
        // correspondingly dot product and vector norm of the query.
        auto const w = ls[i].m_weight + m_maxWeights[i];
        auto const l = ln - ls[i].SqrWeight() - prefix.SqrWeight() + w * w;

        nom = dot + (w - ls[i].m_weight) * tw.m_weight;
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

  void Init() { impl::SortAndMerge(m_tws, m_maxWeights); }

  std::vector<TokenWeightPair> m_tws;
  std::vector<double> m_maxWeights;
  boost::optional<TokenWeightPair> m_prefix;
};
}  // namespace search
