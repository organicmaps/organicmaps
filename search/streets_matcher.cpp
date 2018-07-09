#include "search/streets_matcher.hpp"
#include "search/features_filter.hpp"
#include "search/house_numbers_matcher.hpp"
#include "search/query_params.hpp"

#include "indexer/search_string_utils.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

namespace search
{
namespace
{
bool LessByHash(StreetsMatcher::Prediction const & lhs, StreetsMatcher::Prediction const & rhs)
{
  if (lhs.m_hash != rhs.m_hash)
    return lhs.m_hash < rhs.m_hash;

  if (lhs.m_prob != rhs.m_prob)
    return lhs.m_prob > rhs.m_prob;

  if (lhs.GetNumTokens() != rhs.GetNumTokens())
    return lhs.GetNumTokens() > rhs.GetNumTokens();

  return lhs.m_tokenRange.Begin() < rhs.m_tokenRange.Begin();
}
}  // namespace

// static
void StreetsMatcher::Go(BaseContext const & ctx, FeaturesFilter const & filter,
                        QueryParams const & params, vector<Prediction> & predictions)
{
  size_t const kMaxNumOfImprobablePredictions = 3;
  double const kTailProbability = 0.05;

  predictions.clear();
  FindStreets(ctx, filter, params, predictions);

  if (predictions.empty())
    return;

  sort(predictions.begin(), predictions.end(), &LessByHash);
  predictions.erase(
      unique(predictions.begin(), predictions.end(), my::EqualsBy(&Prediction::m_hash)),
      predictions.end());

  sort(predictions.rbegin(), predictions.rend(), my::LessBy(&Prediction::m_prob));
  while (predictions.size() > kMaxNumOfImprobablePredictions &&
         predictions.back().m_prob < kTailProbability)
  {
    predictions.pop_back();
  }
}

// static
void StreetsMatcher::FindStreets(BaseContext const & ctx, FeaturesFilter const & filter,
                                 QueryParams const & params, vector<Prediction> & predictions)
{
  for (size_t startToken = 0; startToken < ctx.m_numTokens; ++startToken)
  {
    if (ctx.IsTokenUsed(startToken))
      continue;

    // Here we try to match as many tokens as possible while
    // intersection is a non-empty bit vector of streets. Single
    // tokens that are synonyms to streets are ignored.  Moreover,
    // each time a token that looks like a beginning of a house number
    // is met, we try to use current intersection of tokens as a
    // street layer and try to match BUILDINGs or POIs.
    CBV streets(ctx.m_streets);

    CBV all;
    all.SetFull();

    size_t curToken = startToken;

    // This variable is used for prevention of duplicate calls to
    // CreateStreetsLayerAndMatchLowerLayers() with the same
    // arguments.
    size_t lastToken = startToken;

    // When true, no bit vectors were intersected with |streets| at all.
    bool emptyIntersection = true;

    // When true, |streets| is in the incomplete state and can't be
    // used for creation of street layers.
    bool incomplete = false;

    auto emit = [&]()
    {
      if (!streets.IsEmpty() && !emptyIntersection && !incomplete && lastToken != curToken)
      {
        CBV fs(streets);
        CBV fa(all);

        ASSERT(!fs.IsFull(), ());
        ASSERT(!fa.IsFull(), ());

        if (filter.NeedToFilter(fs))
          fs = filter.Filter(fs);

        if (fs.IsEmpty())
          return;

        if (filter.NeedToFilter(fa))
          fa = filter.Filter(fa).Union(fs);

        predictions.emplace_back();
        auto & prediction = predictions.back();

        prediction.m_tokenRange = TokenRange(startToken, curToken);

        ASSERT_NOT_EQUAL(fs.PopCount(), 0, ());
        ASSERT_LESS_OR_EQUAL(fs.PopCount(), fa.PopCount(), ());
        prediction.m_prob = static_cast<double>(fs.PopCount()) / static_cast<double>(fa.PopCount());

        prediction.m_features = move(fs);
        prediction.m_hash = prediction.m_features.Hash();
      }
    };

    StreetTokensFilter filter([&](strings::UniString const & /* token */, size_t tag)
                              {
                                auto buffer = streets.Intersect(ctx.m_features[tag]);
                                if (tag < curToken)
                                {
                                  // This is the case for delayed
                                  // street synonym.  Therefore,
                                  // |streets| is temporarily in the
                                  // incomplete state.
                                  streets = buffer;
                                  all = all.Intersect(ctx.m_features[tag]);
                                  emptyIntersection = false;

                                  incomplete = true;
                                  return;
                                }
                                ASSERT_EQUAL(tag, curToken, ());

                                // |streets| will become empty after
                                // the intersection. Therefore we need
                                // to create streets layer right now.
                                if (buffer.IsEmpty())
                                  emit();

                                streets = buffer;
                                all = all.Intersect(ctx.m_features[tag]);
                                emptyIntersection = false;
                                incomplete = false;
                              });

    for (; curToken < ctx.m_numTokens && !ctx.IsTokenUsed(curToken) && !streets.IsEmpty();
         ++curToken)
    {
      auto const & token = params.GetToken(curToken).m_original;
      bool const isPrefix = params.IsPrefixToken(curToken);

      if (house_numbers::LooksLikeHouseNumber(token, isPrefix))
        emit();

      filter.Put(token, isPrefix, curToken);
    }
    emit();
  }
}
}  // namespace search
