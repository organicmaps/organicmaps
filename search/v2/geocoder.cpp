#include "search/v2/geocoder.hpp"

#include "search/cancel_exception.hpp"
#include "search/retrieval.hpp"
#include "search/search_delimiters.hpp"
#include "search/search_string_utils.hpp"
#include "search/v2/features_layer_matcher.hpp"

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
#include "base/stl_helpers.hpp"

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

// Geocoder::Partition
Geocoder::Partition::Partition() : m_size(0) {}

void Geocoder::Partition::FromFeatures(unique_ptr<coding::CompressedBitVector> features,
                                       Index::FeaturesLoaderGuard & loader,
                                       SearchModel const & model)
{
  for (auto & cluster : m_clusters)
    cluster.clear();

  auto clusterize = [&](uint64_t featureId)
  {
    FeatureType feature;
    loader.GetFeatureByIndex(featureId, feature);
    feature.ParseTypes();
    SearchModel::SearchType searchType = model.GetSearchType(feature);
    if (searchType != SearchModel::SEARCH_TYPE_COUNT)
      m_clusters[searchType].push_back(featureId);
  };

  if (features)
    coding::CompressedBitVectorEnumerator::ForEach(*features, clusterize);

  m_size = 0;
  for (auto const & cluster : m_clusters)
    m_size += cluster.size();
}

// Geocoder::Params --------------------------------------------------------------------------------
Geocoder::Params::Params() : m_maxNumResults(0) {}

// Geocoder::Geocoder ------------------------------------------------------------------------------
Geocoder::Geocoder(Index & index)
  : m_index(index)
  , m_numTokens(0)
  , m_model(SearchModel::Instance())
  , m_value(nullptr)
  , m_filter(static_cast<my::Cancellable const &>(*this))
  , m_finder(static_cast<my::Cancellable const &>(*this))
  , m_results(nullptr)
{
}

Geocoder::~Geocoder() {}

void Geocoder::SetParams(Params const & params)
{
  m_params = params;
  m_retrievalParams = params;

  m_filter.SetViewport(m_params.m_viewport);
  m_filter.SetMaxNumResults(m_params.m_maxNumResults);
  m_filter.SetScale(m_params.m_scale);

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
                       m_matcher.reset();
                       m_loader.reset();
                       m_partitions.clear();
                     });

      m_partitions.clear();
      m_loader.reset(new Index::FeaturesLoaderGuard(m_index, m_mwmId));
      m_matcher.reset(new FeaturesLayerMatcher(
          m_index, m_mwmId, *m_value, m_loader->GetFeaturesVector(), *this /* cancellable */));
      m_filter.SetValue(m_value, m_mwmId);

      m_partitions.resize(m_numTokens);
      for (size_t i = 0; i < m_numTokens; ++i)
      {
        PrepareRetrievalParams(i, i + 1);
        m_partitions[i].FromFeatures(Retrieval::RetrieveAddressFeatures(
                                         *m_value, *this /* cancellable */, m_retrievalParams),
                                     *m_loader, m_model);
      }

      DoGeocoding(0 /* curToken */);
    }
  }
  catch (CancelException & e)
  {
  }
}

void Geocoder::ClearCaches()
{
  m_partitions.clear();
  m_matcher.reset();
}

void Geocoder::PrepareRetrievalParams(size_t curToken, size_t endToken)
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
  BailIfCancelled(static_cast<my::Cancellable const &>(*this));

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
    BailIfCancelled(static_cast<my::Cancellable const &>(*this));

    {
      auto & layer = m_layers.back();
      layer.Clear();
      layer.m_startToken = curToken;
      layer.m_endToken = curToken + n;
      JoinQueryTokens(m_params, layer.m_startToken, layer.m_endToken, " " /* sep */,
                      layer.m_subQuery);
    }

    BailIfCancelled(static_cast<my::Cancellable const &>(*this));

    bool const looksLikeHouseNumber = feature::IsHouseNumber(m_layers.back().m_subQuery);
    auto const & partition = m_partitions[curToken + n - 1];
    if (partition.m_size == 0 && !looksLikeHouseNumber)
      break;

    vector<uint32_t> clusters[SearchModel::SEARCH_TYPE_COUNT];
    vector<uint32_t> buffer;

    for (size_t i = 0; i != SearchModel::SEARCH_TYPE_COUNT; ++i)
    {
      // ATTENTION: DO NOT USE layer after recursive calls to
      // DoGeocoding().  This may lead to use-after-free.
      auto & layer = m_layers.back();

      // Following code intersects posting lists for tokens [curToken,
      // curToken + n). This can be done incrementally, as we have
      // |clusters| to store intersections.
      if (n == 1)
      {
        layer.m_sortedFeatures = &partition.m_clusters[i];
      }
      else if (n == 2)
      {
        clusters[i].clear();
        auto const & first = m_partitions[curToken].m_clusters[i];
        auto const & second = m_partitions[curToken + 1].m_clusters[i];
        set_intersection(first.begin(), first.end(), second.begin(), second.end(),
                         back_inserter(clusters[i]));
        layer.m_sortedFeatures = &clusters[i];
      }
      else
      {
        buffer.clear();
        set_intersection(clusters[i].begin(), clusters[i].end(), partition.m_clusters[i].begin(),
                         partition.m_clusters[i].end(), back_inserter(buffer));
        clusters[i].swap(buffer);
        layer.m_sortedFeatures = &clusters[i];
      }

      if (i == SearchModel::SEARCH_TYPE_BUILDING)
      {
        if (layer.m_sortedFeatures->empty() && !looksLikeHouseNumber)
          continue;
      }
      else if (layer.m_sortedFeatures->empty())
      {
        continue;
      }

      if (i == SearchModel::SEARCH_TYPE_STREET)
      {
        string key;
        GetStreetNameAsKey(layer.m_subQuery, key);
        if (key.empty())
          continue;
      }

      layer.m_type = static_cast<SearchModel::SearchType>(i);
      if (IsLayerSequenceSane())
        DoGeocoding(curToken + n);
    }
  }
}

bool Geocoder::IsLayerSequenceSane() const
{
  ASSERT(!m_layers.empty(), ());
  static_assert(SearchModel::SEARCH_TYPE_COUNT <= 32,
                "Select a wider type to represent search types mask.");
  uint32_t mask = 0;
  size_t buildingIndex = m_layers.size();
  size_t streetIndex = m_layers.size();

  // Following loop returns false iff there're two different layers
  // of the same search type.
  for (size_t i = 0; i < m_layers.size(); ++i)
  {
    auto const & layer = m_layers[i];
    ASSERT_NOT_EQUAL(layer.m_type, SearchModel::SEARCH_TYPE_COUNT, ());

    // TODO (@y): probably it's worth to check belongs-to-locality here.
    uint32_t bit = 1U << layer.m_type;
    if (mask & bit)
      return false;
    mask |= bit;

    if (layer.m_type == SearchModel::SEARCH_TYPE_BUILDING)
      buildingIndex = i;
    if (layer.m_type == SearchModel::SEARCH_TYPE_STREET)
      streetIndex = i;

    // Checks that building and street layers are neighbours.
    if (buildingIndex != m_layers.size() && streetIndex != m_layers.size() &&
        buildingIndex != streetIndex + 1 && streetIndex != buildingIndex + 1)
    {
      return false;
    }
  }

  return true;
}

void Geocoder::FindPaths()
{
  ASSERT(!m_layers.empty(), ());

  // Layers ordered by a search type.
  vector<FeaturesLayer const *> sortedLayers;
  sortedLayers.reserve(m_layers.size());
  for (auto & layer : m_layers)
    sortedLayers.push_back(&layer);
  sort(sortedLayers.begin(), sortedLayers.end(), my::CompareBy(&FeaturesLayer::m_type));

  m_finder.ForEachReachableVertex(*m_matcher, m_filter, sortedLayers, [this](uint32_t featureId)
                                  {
                                    m_results->emplace_back(m_mwmId, featureId);
                                  });
}
}  // namespace v2
}  // namespace search
