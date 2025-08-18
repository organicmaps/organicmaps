#include "search/doc_vec.hpp"

#include <limits>

namespace search
{
using namespace std;

namespace
{
// Accumulates frequencies of equal tokens in |tfs|. Result is sorted
// by tokens.
void SortAndMerge(vector<strings::UniString> tokens, vector<TokenFrequencyPair> & tfs)
{
  ASSERT(tfs.empty(), ());
  sort(tokens.begin(), tokens.end());
  for (size_t i = 0; i < tokens.size(); ++i)
    if (tfs.empty() || tfs.back().m_token != tokens[i])
      tfs.emplace_back(tokens[i], 1 /* frequency */);
    else
      ++tfs.back().m_frequency;
}

double GetTfIdf(double tf, double idf)
{
  return tf * idf;
}

double GetWeightImpl(IdfMap & idfs, TokenFrequencyPair const & tf, bool isPrefix)
{
  return GetTfIdf(tf.m_frequency, idfs.Get(tf.m_token, isPrefix));
}

double GetSqrWeightImpl(IdfMap & idfs, TokenFrequencyPair const & tf, bool isPrefix)
{
  auto const w = GetWeightImpl(idfs, tf, isPrefix);
  return w * w;
}

// Computes squared L2 norm of vector of tokens.
double SqrL2(IdfMap & idfs, vector<TokenFrequencyPair> const & tfs)
{
  double sum = 0;
  for (auto const & tf : tfs)
    sum += GetSqrWeightImpl(idfs, tf, false /* isPrefix */);
  return sum;
}

// Computes squared L2 norm of vector of tokens + prefix token.
double SqrL2(IdfMap & idfs, vector<TokenFrequencyPair> const & tfs, optional<strings::UniString> const & prefix)
{
  auto result = SqrL2(idfs, tfs);
  if (prefix)
    result += GetSqrWeightImpl(idfs, TokenFrequencyPair(*prefix, 1 /* frequency */), true /* isPrefix */);
  return result;
}
}  // namespace

// TokenFrequencyPair ------------------------------------------------------------------------------
bool TokenFrequencyPair::operator<(TokenFrequencyPair const & rhs) const
{
  if (m_token != rhs.m_token)
    return m_token < rhs.m_token;
  return m_frequency < rhs.m_frequency;
}

void TokenFrequencyPair::Swap(TokenFrequencyPair & rhs)
{
  m_token.swap(rhs.m_token);
  swap(m_frequency, rhs.m_frequency);
}

string DebugPrint(TokenFrequencyPair const & tf)
{
  ostringstream os;
  os << "TokenFrequencyPair [" << DebugPrint(tf.m_token) << ", " << tf.m_frequency << "]";
  return os.str();
}

// DocVec ------------------------------------------------------------------------------------------
DocVec::DocVec(Builder const & builder)
{
  SortAndMerge(builder.m_tokens, m_tfs);
}

double DocVec::Norm(IdfMap & idfs) const
{
  return SqrL2(idfs, m_tfs);
}

strings::UniString const & DocVec::GetToken(size_t i) const
{
  ASSERT_LESS(i, m_tfs.size(), ());
  return m_tfs[i].m_token;
}

double DocVec::GetIdf(IdfMap & idfs, size_t i) const
{
  ASSERT_LESS(i, m_tfs.size(), ());
  return idfs.Get(m_tfs[i].m_token, false /* isPrefix */);
}

double DocVec::GetWeight(IdfMap & idfs, size_t i) const
{
  ASSERT_LESS(i, m_tfs.size(), ());
  return GetWeightImpl(idfs, m_tfs[i], false /* isPrefix */);
}

// QueryVec ----------------------------------------------------------------------------------------
QueryVec::QueryVec(IdfMap & idfs, Builder const & builder) : m_idfs(&idfs), m_prefix(builder.m_prefix)
{
  SortAndMerge(builder.m_tokens, m_tfs);
}

double QueryVec::Similarity(IdfMap & docIdfs, DocVec const & rhs)
{
  size_t kInvalidIndex = numeric_limits<size_t>::max();

  if (Empty() && rhs.Empty())
    return 1.0;

  if (Empty() || rhs.Empty())
    return 0.0;

  vector<size_t> rsMatchTo(rhs.GetNumTokens(), kInvalidIndex);

  double dot = 0;
  {
    size_t i = 0, j = 0;

    while (i < m_tfs.size() && j < rhs.GetNumTokens())
    {
      auto const & lt = m_tfs[i].m_token;
      auto const & rt = rhs.GetToken(j);

      if (lt < rt)
      {
        ++i;
      }
      else if (lt > rt)
      {
        ++j;
      }
      else
      {
        dot += GetFullTokenWeight(i) * rhs.GetWeight(docIdfs, j);
        rsMatchTo[j] = i;
        ++i;
        ++j;
      }
    }
  }

  auto const ln = Norm();
  auto const rn = rhs.Norm(docIdfs);

  // This similarity metric assumes that prefix is not matched in the document.
  double const similarityNoPrefix = ln > 0 && rn > 0 ? dot / sqrt(ln) / sqrt(rn) : 0;

  if (!m_prefix)
    return similarityNoPrefix;

  double similarityWithPrefix = 0;
  auto const & prefix = *m_prefix;

  // Let's try to match prefix token with all tokens in the
  // document, and compute the best cosine distance.
  for (size_t j = 0; j < rhs.GetNumTokens(); ++j)
  {
    auto const & t = rhs.GetToken(j);
    if (!strings::StartsWith(t.begin(), t.end(), prefix.begin(), prefix.end()))
      continue;

    auto const i = rsMatchTo[j];

    double num = 0;
    double denom = 0;
    if (i == kInvalidIndex)
    {
      // If this document token is not matched with full tokens in a
      // query, we need to update its weight in the cosine distance
      // - so we need to update correspondingly dot product and
      // vector norms of query and doc.
      auto const oldW = GetPrefixTokenWeight();
      auto const newW = GetTfIdf(1 /* frequency */, rhs.GetIdf(docIdfs, j));
      auto const l = max(0.0, ln - oldW * oldW + newW * newW);

      num = dot + newW * rhs.GetWeight(docIdfs, j);
      denom = sqrt(l) * sqrt(rn);
    }
    else
    {
      // If this document token is already matched with |i|-th full
      // token in a query - we know that completion of the prefix
      // token is the |i|-th query token. So we need to update
      // correspondingly dot product and vector norm of the query.
      auto const oldFW = GetFullTokenWeight(i);
      auto const oldPW = GetPrefixTokenWeight();

      auto const tf = m_tfs[i].m_frequency + 1;
      auto const idf = m_idfs->Get(m_tfs[i].m_token, false /* isPrefix */);
      auto const newW = GetTfIdf(tf, idf);

      auto const l = ln - oldFW * oldFW - oldPW * oldPW + newW * newW;

      num = dot + (newW - oldFW) * rhs.GetWeight(docIdfs, j);
      denom = sqrt(l) * sqrt(rn);
    }

    if (denom > 0)
      similarityWithPrefix = max(similarityWithPrefix, num / denom);
  }

  return max(similarityWithPrefix, similarityNoPrefix);
}

double QueryVec::Norm()
{
  return SqrL2(*m_idfs, m_tfs, m_prefix);
}

double QueryVec::GetFullTokenWeight(size_t i)
{
  ASSERT_LESS(i, m_tfs.size(), ());
  return GetWeightImpl(*m_idfs, m_tfs[i], false /* isPrefix */);
}

double QueryVec::GetPrefixTokenWeight()
{
  ASSERT(m_prefix, ());
  return GetWeightImpl(*m_idfs, TokenFrequencyPair(*m_prefix, 1 /* frequency */), true /* isPrefix */);
}
}  // namespace search
