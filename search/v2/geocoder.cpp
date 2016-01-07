#include "search/v2/geocoder.hpp"

#include "search/retrieval.hpp"
#include "search/search_delimiters.hpp"
#include "search/search_string_utils.hpp"
#include "search/v2/features_layer_matcher.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "platform/preferred_languages.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_add.hpp"
#include "base/stl_helpers.hpp"

#include "std/algorithm.hpp"
#include "std/iterator.hpp"
#include "std/target_os.hpp"

#include "defines.hpp"

#if defined(DEBUG)
#include "base/timer.hpp"
#endif

#if defined(USE_GOOGLE_PROFILER) && defined(OMIM_OS_LINUX)
#include <gperftools/profiler.h>
#endif

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

vector<strings::UniString> GetStreetCategories()
{
  vector<strings::UniString> categories;

  auto const & classificator = classif();
  auto addCategory = [&](uint32_t type)
  {
    uint32_t const index = classificator.GetIndexForType(type);
    categories.push_back(FeatureTypeToString(index));
  };
  ftypes::IsStreetChecker::Instance().ForEachType(addCategory);

  return categories;
}

template <typename TFn>
void ForEachStreetCategory(TFn && fn)
{
  static auto const kCategories = GetStreetCategories();
  for_each(kCategories.begin(), kCategories.end(), forward<TFn>(fn));
}

bool HasSearchIndex(MwmValue const & value) { return value.m_cont.IsExist(SEARCH_INDEX_FILE_TAG); }

bool HasGeometryIndex(MwmValue & value) { return value.m_cont.IsExist(INDEX_FILE_TAG); }

MwmSet::MwmHandle FindWorld(Index & index, vector<shared_ptr<MwmInfo>> & infos)
{
  MwmSet::MwmHandle handle;
  for (auto const & info : infos)
  {
    if (info->GetType() == MwmInfo::WORLD)
    {
      handle = index.GetMwmHandleById(MwmSet::MwmId(info));
      break;
    }
  }
  return handle;
}
}  // namespace

// Geocoder::Params --------------------------------------------------------------------------------
Geocoder::Params::Params() : m_position(0, 0), m_maxNumResults(0) {}

// Geocoder::Geocoder ------------------------------------------------------------------------------
Geocoder::Geocoder(Index & index)
  : m_index(index)
  , m_numTokens(0)
  , m_model(SearchModel::Instance())
  , m_streets(nullptr)
  , m_finder(static_cast<my::Cancellable const &>(*this))
  , m_results(nullptr)
{
}

Geocoder::~Geocoder() {}

void Geocoder::SetParams(Params const & params)
{
  m_params = params;
  m_retrievalParams = params;
  m_numTokens = m_params.m_tokens.size();
  if (!m_params.m_prefixTokens.empty())
    ++m_numTokens;
}

void Geocoder::Go(vector<FeatureID> & results)
{
  // TODO (@y): remove following code as soon as Geocoder::Go() will
  // work fast for most cases (significantly less than 1 second).
#if defined(DEBUG)
  my::Timer timer;
  MY_SCOPE_GUARD(printDuration, [&timer]()
                 {
                   LOG(LINFO, ("Total geocoding time:", timer.ElapsedSeconds(), "seconds"));
                 });
#endif
#if defined(USE_GOOGLE_PROFILER) && defined(OMIM_OS_LINUX)
  ProfilerStart("/tmp/geocoder.prof");
  MY_SCOPE_GUARD(stopProfiler, &ProfilerStop);
#endif

  if (m_numTokens == 0)
    return;

  m_results = &results;

  try
  {
    vector<shared_ptr<MwmInfo>> infos;
    m_index.GetMwmsInfo(infos);

    // Tries to find world and fill localities table.
    {
      m_localities.clear();
      MwmSet::MwmHandle handle = FindWorld(m_index, infos);
      if (handle.IsAlive())
      {
        auto & value = *handle.GetValue<MwmValue>();

        // All MwmIds are unique during the application lifetime, so
        // it's ok to save MwmId.
        m_worldId = handle.GetId();
        if (HasSearchIndex(value))
          FillLocalitiesTable(move(handle));
      }
    }

    auto processCountry = [&](unique_ptr<MwmContext> context)
    {
      ASSERT(context, ());
      m_context = move(context);
      MY_SCOPE_GUARD(cleanup, [&]()
                     {
                       m_matcher.reset();
                       m_context.reset();
                       m_features.clear();
                       m_streets = nullptr;
                     });

      m_matcher.reset(new FeaturesLayerMatcher(m_index, *m_context, *this /* cancellable */));

      // Creates a cache of posting lists for each token.
      m_features.resize(m_numTokens);
      for (size_t i = 0; i < m_numTokens; ++i)
      {
        PrepareRetrievalParams(i, i + 1);
        m_features[i] = Retrieval::RetrieveAddressFeatures(
            m_context->m_value, *this /* cancellable */, m_retrievalParams);
        ASSERT(m_features[i], ());
      }

      m_streets = LoadStreets(*m_context);

      DoGeocodingWithLocalities();
      DoGeocodingWithoutLocalities();
    };

    // Iterates through all alive mwms and performs geocoding.
    ForEachCountry(infos, processCountry);
  }
  catch (CancelException & e)
  {
  }
}

void Geocoder::ClearCaches()
{
  m_features.clear();
  m_matcher.reset();
  m_streetsCache.clear();
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

void Geocoder::FillLocalitiesTable(MwmContext const & context)
{
  m_localities.clear();

  auto addLocality = [&](size_t curToken, size_t endToken, uint32_t featureId)
  {
    FeatureType ft;
    context.m_vector.GetByIndex(featureId, ft);
    if (m_model.GetSearchType(ft) != SearchModel::SEARCH_TYPE_CITY)
      return;

    m2::PointD const center = feature::GetCenter(ft, FeatureType::WORST_GEOMETRY);
    double const radiusM = ftypes::GetRadiusByPopulation(ft.GetPopulation());

    Locality locality;
    locality.m_featureId = featureId;
    locality.m_startToken = curToken;
    locality.m_endToken = endToken;
    locality.m_rect = MercatorBounds::RectByCenterXYAndSizeInMeters(center, radiusM);
    m_localities[make_pair(curToken, endToken)].push_back(locality);
  };

  for (size_t curToken = 0; curToken < m_numTokens; ++curToken)
  {
    for (size_t endToken = curToken + 1; endToken <= m_numTokens; ++endToken)
    {
      PrepareRetrievalParams(curToken, endToken);
      auto localities = Retrieval::RetrieveAddressFeatures(
          context.m_value, static_cast<my::Cancellable const &>(*this), m_retrievalParams);
      if (coding::CompressedBitVector::IsEmpty(localities))
        break;
      coding::CompressedBitVectorEnumerator::ForEach(*localities,
                                                     bind(addLocality, curToken, endToken, _1));
    }
  }
}

template <typename TFn>
void Geocoder::ForEachCountry(vector<shared_ptr<MwmInfo>> const & infos, TFn && fn)
{
  for (auto const & info : infos)
  {
    if (info->GetType() != MwmInfo::COUNTRY)
      continue;
    auto handle = m_index.GetMwmHandleById(MwmSet::MwmId(info));
    if (!handle.IsAlive())
      continue;
    auto & value = *handle.GetValue<MwmValue>();
    if (!HasSearchIndex(value) || !HasGeometryIndex(value))
      continue;
    fn(make_unique<MwmContext>(move(handle)));
  }
}

void Geocoder::DoGeocodingWithLocalities()
{
  ASSERT(m_context, ("Mwm context must be initialized at this moment."));

  m_usedTokens.assign(m_numTokens, false);

  m2::RectD const countryBounds = m_context->m_value.GetHeader().GetBounds();

  // Localities are ordered my (m_startToken, m_endToken) pairs.
  for (auto const & p : m_localities)
  {
    BailIfCancelled();

    size_t const startToken = p.first.first;
    size_t const endToken = p.first.second;
    if (startToken == 0 && endToken == m_numTokens)
    {
      // Localities match to search query.
      for (auto const & locality : p.second)
        m_results->emplace_back(m_worldId, locality.m_featureId);
      continue;
    }

    // Unites features from all localities and uses the resulting bit
    // vector as a filter for features retrieved during geocoding.
    unique_ptr<coding::CompressedBitVector> allFeatures;
    for (auto const & locality : p.second)
    {
      m2::RectD rect = countryBounds;
      if (!rect.Intersect(locality.m_rect))
        continue;
      auto features = Retrieval::RetrieveGeometryFeatures(
          m_context->m_value, static_cast<my::Cancellable const &>(*this), rect, m_params.m_scale);
      if (!features)
        continue;

      if (!allFeatures)
        allFeatures = move(features);
      else
        allFeatures = coding::CompressedBitVector::Union(*allFeatures, *features);
    }

    if (coding::CompressedBitVector::IsEmpty(allFeatures))
      continue;

    m_filter.SetFilter(move(allFeatures));

    // Filter will be applied for all non-empty bit vectors.
    m_filter.SetThreshold(0);

    // Marks all tokens matched to localities as used and performs geocoding.
    fill(m_usedTokens.begin() + startToken, m_usedTokens.begin() + endToken, true);
    GreedilyMatchStreets();
    fill(m_usedTokens.begin() + startToken, m_usedTokens.begin() + endToken, false);
  }
}

void Geocoder::DoGeocodingWithoutLocalities()
{
  // 50km maximum viewport radius.
  double constexpr kMaxViewportRadiusM = 50.0 * 1000;

  // 50km radius around position.
  double constexpr kMaxPositionRadiusM = 50.0 * 1000;

  double constexpr kEps = 1.0e-5;

  m2::RectD const & viewport = m_params.m_viewport;
  m2::PointD const & position = m_params.m_position;

  // Extracts features in viewport.
  unique_ptr<coding::CompressedBitVector> viewportFeatures;
  {
    // Limits viewport by kMaxViewportRadiusM.
    m2::RectD const viewportLimit =
        MercatorBounds::RectByCenterXYAndSizeInMeters(viewport.Center(), kMaxViewportRadiusM);
    m2::RectD rect = viewport;
    rect.Intersect(viewportLimit);
    if (!rect.IsEmptyInterior())
    {
      viewportFeatures = Retrieval::RetrieveGeometryFeatures(
          m_context->m_value, static_cast<my::Cancellable const &>(*this), rect, m_params.m_scale);
    }
  }

  // Extracts features around user position.
  unique_ptr<coding::CompressedBitVector> positionFeatures;
  if (!position.EqualDxDy(viewport.Center(), kEps))
  {
    m2::RectD const rect =
        MercatorBounds::RectByCenterXYAndSizeInMeters(position, kMaxPositionRadiusM);
    positionFeatures = Retrieval::RetrieveGeometryFeatures(
        m_context->m_value, static_cast<my::Cancellable const &>(*this), rect, m_params.m_scale);
  }

  if (coding::CompressedBitVector::IsEmpty(viewportFeatures) &&
      coding::CompressedBitVector::IsEmpty(positionFeatures))
  {
    m_filter.SetFilter(nullptr);
  }
  else if (coding::CompressedBitVector::IsEmpty(viewportFeatures))
  {
    m_filter.SetFilter(move(positionFeatures));
  }
  else if (coding::CompressedBitVector::IsEmpty(positionFeatures))
  {
    m_filter.SetFilter(move(viewportFeatures));
  }
  else
  {
    m_filter.SetFilter(coding::CompressedBitVector::Union(*viewportFeatures, *positionFeatures));
  }

  // Filter will be applied only for large bit vectors.
  m_filter.SetThreshold(m_params.m_maxNumResults);
  GreedilyMatchStreets();
}

void Geocoder::GreedilyMatchStreets()
{
  MatchPOIsAndBuildings(0 /* curToken */);

  ASSERT(m_layers.empty(), ());

  m_layers.emplace_back();

  MY_SCOPE_GUARD(cleanupGuard, bind(&vector<FeaturesLayer>::pop_back, &m_layers));
  for (size_t startToken = 0; startToken < m_numTokens; ++startToken)
  {
    if (m_usedTokens[startToken])
      continue;

    unique_ptr<coding::CompressedBitVector> buffer;
    unique_ptr<coding::CompressedBitVector> allFeatures;

    size_t curToken = startToken;
    for (; curToken < m_numTokens && !m_usedTokens[curToken]; ++curToken)
    {
      if (IsStreetSynonym(strings::ToUtf8(m_params.GetTokens(curToken).front())))
        continue;
      if (startToken == curToken || coding::CompressedBitVector::IsEmpty(allFeatures))
      {
        buffer = coding::CompressedBitVector::Intersect(*m_streets, *m_features[curToken]);
        if (m_filter.NeedToFilter(*buffer))
          buffer = m_filter.Filter(*buffer);
      }
      else
      {
        buffer = coding::CompressedBitVector::Intersect(*allFeatures, *m_features[curToken]);
      }
      if (coding::CompressedBitVector::IsEmpty(buffer))
          break;
      allFeatures.swap(buffer);
    }

    if (coding::CompressedBitVector::IsEmpty(allFeatures))
      continue;

    auto & layer = m_layers.back();
    layer.Clear();
    layer.m_type = SearchModel::SEARCH_TYPE_STREET;
    layer.m_startToken = startToken;
    layer.m_endToken = curToken;
    JoinQueryTokens(m_params, layer.m_startToken, layer.m_endToken, " " /* sep */,
                    layer.m_subQuery);
    vector<uint32_t> sortedFeatures;
    coding::CompressedBitVectorEnumerator::ForEach(*allFeatures,
                                                   MakeBackInsertFunctor(sortedFeatures));
    layer.m_sortedFeatures = &sortedFeatures;

    fill(m_usedTokens.begin() + startToken, m_usedTokens.begin() + curToken, true);
    MatchPOIsAndBuildings(0 /* curToken */);
    fill(m_usedTokens.begin() + startToken, m_usedTokens.begin() + curToken, false);
  }
}

void Geocoder::MatchPOIsAndBuildings(size_t curToken)
{
  // Skip used tokens.
  while (curToken != m_numTokens && m_usedTokens[curToken])
    ++curToken;

  BailIfCancelled();

  if (curToken == m_numTokens)
  {
    // All tokens were consumed, find paths through layers, emit
    // features.
    FindPaths();
    return;
  }

  m_layers.emplace_back();
  MY_SCOPE_GUARD(cleanupGuard, bind(&vector<FeaturesLayer>::pop_back, &m_layers));

  // Clusters of features by search type. Each cluster is a sorted
  // list of ids.
  vector<uint32_t> clusters[SearchModel::SEARCH_TYPE_STREET];

  auto clusterize = [&](uint32_t featureId)
  {
    if (m_streets->GetBit(featureId))
      return;

    FeatureType feature;
    m_context->m_vector.GetByIndex(featureId, feature);
    feature.ParseTypes();
    SearchModel::SearchType const searchType = m_model.GetSearchType(feature);

    // All SEARCH_TYPE_CITY features were filtered in DoGeocodingWithLocalities().
    // All SEARCH_TYPE_STREET features were filtered in GreedyMatchStreets().
    if (searchType < SearchModel::SEARCH_TYPE_STREET)
      clusters[searchType].push_back(featureId);
  };

  unique_ptr<coding::CompressedBitVector> intersection;
  coding::CompressedBitVector * features = nullptr;

  // Try to consume first n tokens starting at |curToken|.
  for (size_t n = 1; curToken + n <= m_numTokens && !m_usedTokens[curToken + n - 1]; ++n)
  {
    // At this point |intersection| is the intersection of
    // m_features[curToken], m_features[curToken + 1], ...,
    // m_features[curToken + n - 2], iff n > 2.

    BailIfCancelled();

    {
      auto & layer = m_layers.back();
      layer.Clear();
      layer.m_startToken = curToken;
      layer.m_endToken = curToken + n;
      JoinQueryTokens(m_params, layer.m_startToken, layer.m_endToken, " " /* sep */,
                      layer.m_subQuery);
    }

    if (n == 1)
    {
      features = m_features[curToken].get();
      if (m_filter.NeedToFilter(*features))
      {
        intersection = m_filter.Filter(*features);
        features = intersection.get();
      }
    }
    else
    {
      intersection =
          coding::CompressedBitVector::Intersect(*features, *m_features[curToken + n - 1]);
      features = intersection.get();
    }
    ASSERT(features, ());

    bool const looksLikeHouseNumber = feature::IsHouseNumber(m_layers.back().m_subQuery);

    if (coding::CompressedBitVector::IsEmpty(features) && !looksLikeHouseNumber)
      break;

    for (auto & cluster : clusters)
      cluster.clear();
    coding::CompressedBitVectorEnumerator::ForEach(*features, clusterize);

    for (size_t i = 0; i < ARRAY_SIZE(clusters); ++i)
    {
      // ATTENTION: DO NOT USE layer after recursive calls to
      // DoGeocoding().  This may lead to use-after-free.
      auto & layer = m_layers.back();
      layer.m_sortedFeatures = &clusters[i];

      if (i == SearchModel::SEARCH_TYPE_BUILDING)
      {
        if (layer.m_sortedFeatures->empty() && !looksLikeHouseNumber)
          continue;
      }
      else if (layer.m_sortedFeatures->empty())
      {
        continue;
      }

      layer.m_type = static_cast<SearchModel::SearchType>(i);
      if (IsLayerSequenceSane())
        MatchPOIsAndBuildings(curToken + n);
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
    if (buildingIndex != m_layers.size() && streetIndex != m_layers.size())
    {
      auto & buildings = m_layers[buildingIndex];
      auto & streets = m_layers[streetIndex];
      if (buildings.m_startToken != streets.m_endToken &&
          buildings.m_endToken != streets.m_startToken)
      {
        return false;
      }
    }
  }

  return true;
}

void Geocoder::FindPaths()
{
  if (m_layers.empty())
    return;

  // Layers ordered by a search type.
  vector<FeaturesLayer const *> sortedLayers;
  sortedLayers.reserve(m_layers.size());
  for (auto & layer : m_layers)
    sortedLayers.push_back(&layer);
  sort(sortedLayers.begin(), sortedLayers.end(), my::CompareBy(&FeaturesLayer::m_type));

  m_finder.ForEachReachableVertex(*m_matcher, sortedLayers, [this](uint32_t featureId)
                                  {
                                    m_results->emplace_back(m_context->m_id, featureId);
                                  });
}

coding::CompressedBitVector const * Geocoder::LoadStreets(MwmContext & context)
{
  if (!context.m_handle.IsAlive() || !HasSearchIndex(context.m_value))
    return nullptr;

  auto mwmId = context.m_handle.GetId();
  auto const it = m_streetsCache.find(mwmId);
  if (it != m_streetsCache.cend())
    return it->second.get();

  unique_ptr<coding::CompressedBitVector> allStreets;

  m_retrievalParams.m_tokens.resize(1);
  m_retrievalParams.m_tokens[0].resize(1);
  m_retrievalParams.m_prefixTokens.clear();

  vector<unique_ptr<coding::CompressedBitVector>> streetsList;
  ForEachStreetCategory([&](strings::UniString const & category)
                        {
                          m_retrievalParams.m_tokens[0][0] = category;
                          auto streets = Retrieval::RetrieveAddressFeatures(
                              context.m_value, *this /* cancellable */, m_retrievalParams);
                          if (!coding::CompressedBitVector::IsEmpty(streets))
                            streetsList.push_back(move(streets));
                        });

  // Following code performs pairwise union of adjacent bit vectors
  // until at most one bit vector is left.
  while (streetsList.size() > 1)
  {
    size_t i = 0;
    size_t j = 0;
    for (; j + 1 < streetsList.size(); j += 2)
      streetsList[i++] = coding::CompressedBitVector::Union(*streetsList[j], *streetsList[j + 1]);
    for (; j < streetsList.size(); ++j)
      streetsList[i++] = move(streetsList[j]);
    streetsList.resize(i);
  }

  if (streetsList.empty())
    streetsList.push_back(make_unique<coding::DenseCBV>());
  auto const * result = streetsList[0].get();
  m_streetsCache[mwmId] = move(streetsList[0]);
  return result;
}
}  // namespace v2
}  // namespace search
