#include "search/v2/geocoder.hpp"

#include "search/retrieval.hpp"
#include "search/v2/features_layer_path_finder.hpp"
#include "search/search_delimiters.hpp"
#include "search/search_string_utils.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/index.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "platform/preferred_languages.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_add.hpp"

#include "std/algorithm.hpp"
#include "std/iterator.hpp"

namespace search
{
namespace v2
{
namespace
{
void JoinQueryTokens(SearchQueryParams const & params, size_t curToken, size_t endToken,
                     string const & sep, string & res)
{
  ASSERT_LESS_OR_EQUAL(curToken, endToken, ());
  for (size_t i = curToken; i < endToken; ++i)
  {
    if (i < params.m_tokens.size())
    {
      res.append(strings::ToUtf8(params.m_tokens[i].front()));
    }
    else
    {
      ASSERT_EQUAL(i, params.m_tokens.size(), ());
      res.append(strings::ToUtf8(params.m_prefixTokens.front()));
    }

    if (i + 1 != endToken)
      res.append(sep);
  }
}
}  // namespace

Geocoder::Geocoder(Index & index)
  : m_index(index)
  , m_numTokens(0)
  , m_model(SearchModel::Instance())
  , m_value(nullptr)
  , m_results(nullptr)
{
}

Geocoder::~Geocoder() {}

void Geocoder::SetSearchQueryParams(SearchQueryParams const & params)
{
  m_params = params;
  m_retrievalParams = params;

  m_numTokens = m_params.m_tokens.size();
  if (!m_params.m_prefixTokens.empty())
    ++m_numTokens;
}

void Geocoder::Go(vector<FeatureID> & results)
{
  if (m_numTokens == 0)
    return;

  m_results = &results;

  try
  {
    vector<shared_ptr<MwmInfo>> mwmsInfo;
    m_index.GetMwmsInfo(mwmsInfo);

    // Iterate through all alive mwms and perform geocoding.
    for (auto const & info : mwmsInfo)
    {
      auto handle = m_index.GetMwmHandleById(MwmSet::MwmId(info));
      if (!handle.IsAlive())
        continue;

      m_value = handle.GetValue<MwmValue>();
      if (!m_value || !m_value->m_cont.IsExist(SEARCH_INDEX_FILE_TAG))
        continue;

      m_mwmId = handle.GetId();

      MY_SCOPE_GUARD(cleanup, [&]()
      {
        m_finder.reset();
        m_loader.reset();
        m_cache.clear();
      });

      m_loader.reset(new Index::FeaturesLoaderGuard(m_index, m_mwmId));
      m_finder.reset(new FeaturesLayerPathFinder(*m_value, m_loader->GetFeaturesVector()));
      m_cache.clear();

      DoGeocoding(0 /* curToken */);
    }
  }
  catch (CancelException & e)
  {
  }
}

void Geocoder::PrepareParams(size_t curToken, size_t endToken)
{
  ASSERT_LESS(curToken, endToken, ());
  ASSERT_LESS_OR_EQUAL(endToken, m_numTokens, ());

  m_retrievalParams.m_tokens.clear();
  m_retrievalParams.m_prefixTokens.clear();

  // TODO (@y): possibly it's not cheap to copy vectors of strings.
  // Profile it, and in case of serious performance loss, refactor
  // SearchQueryParams to support subsets of tokens.
  for (size_t i = curToken; i < endToken; ++i)
  {
    if (i < m_params.m_tokens.size())
      m_retrievalParams.m_tokens.push_back(m_params.m_tokens[i]);
    else
      m_retrievalParams.m_prefixTokens = m_params.m_prefixTokens;
  }
}

void Geocoder::DoGeocoding(size_t curToken)
{
  if (curToken == m_numTokens)
  {
    // All tokens were consumed, find paths through layers, emit
    // features.
    FindPaths();
    return;
  }

  m_layers.emplace_back();
  MY_SCOPE_GUARD(cleanupGuard, bind(&vector<FeaturesLayer>::pop_back, &m_layers));

  // Try to consume first n tokens starting at |curToken|.
  for (size_t n = 1; curToken + n <= m_numTokens; ++n)
  {
    PrepareParams(curToken, curToken + n);
    {
      auto & layer = m_layers.back();
      layer.Clear();
      layer.m_startToken = curToken;
      layer.m_endToken = curToken + n;
    }

    // TODO (@y, @m): as |n| increases, good optimization is to update
    // |features| incrementally, from [curToken, curToken + n) to
    // [curToken, curToken + n + 1).
    auto features = RetrieveAddressFeatures(curToken, curToken + n);
    if (!features || features->PopCount() == 0)
      continue;

    vector<uint32_t> clusters[SearchModel::SEARCH_TYPE_COUNT];
    auto clusterize = [&](uint64_t featureId)
    {
      FeatureType feature;
      m_loader->GetFeatureByIndex(featureId, feature);
      feature.ParseTypes();
      SearchModel::SearchType searchType = m_model.GetSearchType(feature);
      if (searchType != SearchModel::SEARCH_TYPE_COUNT)
        clusters[searchType].push_back(featureId);
    };

    coding::CompressedBitVectorEnumerator::ForEach(*features, clusterize);

    bool const looksLikeHouseNumber = LooksLikeHouseNumber(curToken, curToken + n);

    for (size_t i = 0; i != SearchModel::SEARCH_TYPE_COUNT; ++i)
    {
      if (i == SearchModel::SEARCH_TYPE_BUILDING)
      {
        if (clusters[i].empty() && !looksLikeHouseNumber)
          continue;
      }
      else if (clusters[i].empty())
      {
        continue;
      }

      // ATTENTION: DO NOT USE layer after recursive calls to
      // DoGeocoding().  This may lead to use-after-free.
      auto & layer = m_layers.back();

      layer.m_sortedFeatures.swap(clusters[i]);
      ASSERT(is_sorted(layer.m_sortedFeatures.begin(), layer.m_sortedFeatures.end()), ());
      layer.m_type = static_cast<SearchModel::SearchType>(i);
      if (IsLayerSequenceSane())
        DoGeocoding(curToken + n);
    }
  }
}

coding::CompressedBitVector * Geocoder::RetrieveAddressFeatures(size_t curToken, size_t endToken)
{
  uint64_t const key = (static_cast<uint64_t>(curToken) << 32) | static_cast<uint64_t>(endToken);
  if (m_cache.find(key) == m_cache.end())
  {
    m_cache[key] =
        Retrieval::RetrieveAddressFeatures(m_value, *this /* cancellable */, m_retrievalParams);
  }
  return m_cache[key].get();
}

bool Geocoder::IsLayerSequenceSane() const
{
  ASSERT(!m_layers.empty(), ());
  static_assert(SearchModel::SEARCH_TYPE_COUNT <= 32,
                "Select a wider type to represent search types mask.");
  uint32_t mask = 0;
  for (auto const & layer : m_layers)
  {
    ASSERT_NOT_EQUAL(layer.m_type, SearchModel::SEARCH_TYPE_COUNT, ());

    // TODO (@y): probably it's worth to check belongs-to-locality here.

    uint32_t bit = 1U << layer.m_type;
    if (mask & bit)
      return false;
    mask |= bit;
  }
  return true;
}

bool Geocoder::LooksLikeHouseNumber(size_t curToken, size_t endToken) const
{
  string res;
  JoinQueryTokens(m_params, curToken, endToken, " " /* sep */, res);

  // TODO (@y): we need to implement a better check here.
  return feature::IsHouseNumber(res);
}

void Geocoder::FindPaths()
{
  ASSERT(!m_layers.empty(), ());

  auto const compareByType = [](FeaturesLayer const * lhs, FeaturesLayer const * rhs)
  {
    return lhs->m_type < rhs->m_type;
  };

  // Layers ordered by a search type.
  vector<FeaturesLayer *> sortedLayers;
  sortedLayers.reserve(m_layers.size());
  for (auto & layer : m_layers)
    sortedLayers.push_back(&layer);
  sort(sortedLayers.begin(), sortedLayers.end(), compareByType);

  m_finder->ForEachReachableVertex(sortedLayers, [this](uint32_t featureId)
  {
    m_results->emplace_back(m_mwmId, featureId);
  });
}
}  // namespace v2
}  // namespace search
