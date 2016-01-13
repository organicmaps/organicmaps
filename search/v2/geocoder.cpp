#include "search/v2/geocoder.hpp"

#include "search/dummy_rank_table.hpp"
#include "search/retrieval.hpp"
#include "search/search_delimiters.hpp"
#include "search/search_string_utils.hpp"
#include "search/v2/cbv_ptr.hpp"
#include "search/v2/features_layer_matcher.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/rank_table.hpp"

#include "storage/country_info_getter.hpp"

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
size_t constexpr kMaxNumCities = 5;
size_t constexpr kMaxNumCountries = 5;
size_t constexpr kMaxNumLocalities = kMaxNumCities + kMaxNumCountries;
double constexpr kComparePoints = MercatorBounds::GetCellID2PointAbsEpsilon();

template <typename T>
struct Id
{
  T const & operator()(T const & t) const { return t; }
};

struct ScopedMarkTokens
{
  ScopedMarkTokens(vector<bool> & usedTokens, size_t from, size_t to)
    : m_usedTokens(usedTokens), m_from(from), m_to(to)
  {
    ASSERT_LESS_OR_EQUAL(m_from, m_to, ());
    ASSERT_LESS_OR_EQUAL(m_to, m_usedTokens.size(), ());
    fill(m_usedTokens.begin() + m_from, m_usedTokens.begin() + m_to, true /* used */);
  }

  ~ScopedMarkTokens()
  {
    fill(m_usedTokens.begin() + m_from, m_usedTokens.begin() + m_to, false /* used */);
  }

  vector<bool> & m_usedTokens;
  size_t const m_from;
  size_t const m_to;
};

struct LazyRankTable
{
  LazyRankTable(MwmValue const & value) : m_value(value) {}

  uint8_t Get(uint64_t i)
  {
    if (!m_table)
    {
      m_table = search::RankTable::Load(m_value.m_cont);
      if (!m_table)
        m_table = make_unique<search::DummyRankTable>();
    }
    return m_table->Get(i);
  }

  MwmValue const & m_value;
  unique_ptr<search::RankTable> m_table;
};

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

bool HasAllSubstrings(string const & s, vector<string> const & substrs)
{
  for (auto const & substr : substrs)
  {
    if (s.find(substr) == string::npos)
      return false;
  }
  return true;
}

void GetEnglishName(FeatureType const & ft, string & name)
{
  static vector<string> const kUSA{"united", "states", "america"};
  static vector<string> const kUK{"united", "kingdom"};
  static vector<int8_t> const kLangs = {StringUtf8Multilang::GetLangIndex("en"),
                                        StringUtf8Multilang::GetLangIndex("int_name"),
                                        StringUtf8Multilang::GetLangIndex("default")};

  for (auto const & lang : kLangs)
  {
    if (!ft.GetName(lang, name))
      continue;
    strings::AsciiToLower(name);
    if (HasAllSubstrings(name, kUSA))
      name = "usa";
    else if (HasAllSubstrings(name, kUK))
      name = "uk";
    else
      return;
  }
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
Geocoder::Geocoder(Index & index, storage::CountryInfoGetter const & infoGetter)
  : m_index(index)
  , m_infoGetter(infoGetter)
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
      m_cities.clear();
      m_countries.clear();
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
                       m_addressFeatures.clear();
                       m_streets = nullptr;
                     });

      m_matcher.reset(new FeaturesLayerMatcher(m_index, *m_context, *this /* cancellable */));

      // Creates a cache of posting lists for each token.
      m_addressFeatures.resize(m_numTokens);
      for (size_t i = 0; i < m_numTokens; ++i)
      {
        PrepareRetrievalParams(i, i + 1);
        m_addressFeatures[i] = Retrieval::RetrieveAddressFeatures(
            m_context->m_value, *this /* cancellable */, m_retrievalParams);
        ASSERT(m_addressFeatures[i], ());
      }

      m_streets = LoadStreets(*m_context);

      m_usedTokens.assign(m_numTokens, false);
      MatchCountries();
      MatchViewportAndPosition();
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
  m_geometryFeatures.clear();
  m_addressFeatures.clear();
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
  // 1. Get cbv for every single token and prefix.
  vector<unique_ptr<coding::CompressedBitVector>> tokensCBV;
  for (size_t i = 0; i < m_numTokens; ++i)
  {
    PrepareRetrievalParams(i, i + 1);
    tokensCBV.push_back(Retrieval::RetrieveAddressFeatures(
        context.m_value, static_cast<my::Cancellable const &>(*this), m_retrievalParams));
  }

  // 2. Get all locality candidates for the continuous token ranges.
  vector<Locality> preLocalities;

  for (size_t i = 0; i < m_numTokens; ++i)
  {
    CBVPtr intersection;
    intersection.Set(tokensCBV[i].get(), false /*isOwner*/);
    if (intersection.IsEmpty())
      continue;

    for (size_t j = i + 1; j <= m_numTokens; ++j)
    {
      coding::CompressedBitVectorEnumerator::ForEach(*intersection.Get(),
                                                     [&](uint32_t featureId)
                                                     {
                                                       Locality l;
                                                       l.m_featureId = featureId;
                                                       l.m_startToken = i;
                                                       l.m_endToken = j;
                                                       preLocalities.push_back(l);
                                                     });

      if (j < m_numTokens)
      {
        intersection.Intersect(tokensCBV[j].get());
        if (intersection.IsEmpty())
          break;
      }
    }
  }

  auto const tokensCountFn = [&](Locality const & l)
  {
    // Important! Don't take into account matched prefix for locality comparison.
    size_t d = l.m_endToken - l.m_startToken;
    ASSERT_GREATER(d, 0, ());
    if (l.m_endToken == m_numTokens && !m_params.m_prefixTokens.empty())
      --d;
    return d;
  };

  // 3. Unique preLocalities with featureId but leave the longest range if equal.
  sort(preLocalities.begin(), preLocalities.end(),
       [&](Locality const & l1, Locality const & l2)
       {
         if (l1.m_featureId != l2.m_featureId)
           return l1.m_featureId < l2.m_featureId;
         return tokensCountFn(l1) > tokensCountFn(l2);
       });

  preLocalities.erase(unique(preLocalities.begin(), preLocalities.end(),
                             [](Locality const & l1, Locality const & l2)
                             {
                               return l1.m_featureId == l2.m_featureId;
                             }),
                      preLocalities.end());

  LazyRankTable rankTable(context.m_value);

  // 4. Leave most popular localities.
  if (preLocalities.size() > kMaxNumLocalities)
  {
    nth_element(preLocalities.begin(), preLocalities.begin() + kMaxNumLocalities,
                preLocalities.end(),
                [&](Locality const & l1, Locality const & l2)
                {
                  auto const d1 = tokensCountFn(l1);
                  auto const d2 = tokensCountFn(l2);
                  if (d1 != d2)
                    return d1 > d2;
                  return rankTable.Get(l1.m_featureId) > rankTable.Get(l2.m_featureId);
                });
    preLocalities.resize(kMaxNumLocalities);
  }

  // 5. Fill result container.
  size_t numCities = 0;
  size_t numCountries = 0;
  for (auto & l : preLocalities)
  {
    FeatureType ft;
    context.m_vector.GetByIndex(l.m_featureId, ft);

    switch (m_model.GetSearchType(ft))
    {
    case SearchModel::SEARCH_TYPE_CITY:
    {
      if (numCities < kMaxNumCities && ft.GetFeatureType() == feature::GEOM_POINT)
      {
        ++numCities;
        City city = l;
        city.m_rect = MercatorBounds::RectByCenterXYAndSizeInMeters(
            ft.GetCenter(), ftypes::GetRadiusByPopulation(ft.GetPopulation()));

        m_cities[make_pair(l.m_startToken, l.m_endToken)].push_back(city);
      }
      break;
    }
    case SearchModel::SEARCH_TYPE_COUNTRY:
    {
      if (numCountries < kMaxNumCountries && ft.GetFeatureType() == feature::GEOM_POINT)
      {
        ++numCountries;
        Country country(l);
        GetEnglishName(ft, country.m_enName);
        m_infoGetter.GetMatchedRegions(country.m_enName, country.m_regions);
        m_countries[make_pair(l.m_startToken, l.m_endToken)].push_back(country);
      }
      break;
    default:
      break;
    }
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

void Geocoder::MatchCountries()
{
  // Try to skip countries matching.
  MatchCities();

  auto const & fileName = m_context->m_handle.GetId().GetInfo()->GetCountryName();

  // Try to match countries.
  for (auto const & p : m_countries)
  {
    BailIfCancelled();

    size_t const startToken = p.first.first;
    size_t const endToken = p.first.second;
    if (HasUsedTokensInRange(startToken, endToken))
      continue;

    ScopedMarkTokens mark(m_usedTokens, startToken, endToken);
    if (AllTokensUsed())
    {
      // Countries match to search query.
      for (auto const & country : p.second)
        m_results->emplace_back(m_worldId, country.m_featureId);
      continue;
    }

    bool matches = false;
    for (auto const & country : p.second)
    {
      if (m_infoGetter.IsBelongToRegions(fileName, country.m_regions))
      {
        matches = true;
        break;
      }
    }

    if (matches)
      MatchCities();
  }
}

void Geocoder::MatchCities()
{
  m2::RectD const countryBounds = m_context->m_value.GetHeader().GetBounds();

  // Localities are ordered my (m_startToken, m_endToken) pairs.
  for (auto const & p : m_cities)
  {
    BailIfCancelled();

    size_t const startToken = p.first.first;
    size_t const endToken = p.first.second;
    if (HasUsedTokensInRange(startToken, endToken))
      continue;

    ScopedMarkTokens mark(m_usedTokens, startToken, endToken);
    if (AllTokensUsed())
    {
      // Localities match to search query.
      for (auto const & city : p.second)
        m_results->emplace_back(m_worldId, city.m_featureId);
      continue;
    }

    // Unites features from all localities and uses the resulting bit
    // vector as a filter for features retrieved during geocoding.
    CBVPtr allFeatures;
    for (auto const & city : p.second)
    {
      m2::RectD rect = city.m_rect;
      if (!rect.Intersect(countryBounds))
        continue;

      allFeatures.Union(RetrieveGeometryFeatures(*m_context, rect, city.m_featureId));
    }

    if (allFeatures.IsEmpty())
      continue;

    m_filter.SetFilter(allFeatures.Get());
    MY_SCOPE_GUARD(resetFilter, [&]() {m_filter.SetFilter(nullptr);});

    // Filter will be applied for all non-empty bit vectors.
    m_filter.SetThreshold(0);

    GreedilyMatchStreets();
  }
}

void Geocoder::MatchViewportAndPosition()
{
  // 50km maximum viewport radius.
  double constexpr kMaxViewportRadiusM = 50.0 * 1000;

  // 50km radius around position.
  double constexpr kMaxPositionRadiusM = 50.0 * 1000;

  m2::RectD viewport = m_params.m_viewport;
  m2::PointD const & position = m_params.m_position;

  CBVPtr allFeatures;

  // Extracts features in viewport.
  {
    // Limits viewport by kMaxViewportRadiusM.
    m2::RectD const viewportLimit =
        MercatorBounds::RectByCenterXYAndSizeInMeters(viewport.Center(), kMaxViewportRadiusM);
    VERIFY(viewport.Intersect(viewportLimit), ());

    allFeatures.Union(RetrieveGeometryFeatures(*m_context, viewport, VIEWPORT_ID));
  }

  // Extracts features around user position.
  if (!position.EqualDxDy(viewport.Center(), kComparePoints))
  {
    m2::RectD const rect =
        MercatorBounds::RectByCenterXYAndSizeInMeters(position, kMaxPositionRadiusM);

    allFeatures.Union(RetrieveGeometryFeatures(*m_context, rect, POSITION_ID));
  }

  m_filter.SetFilter(allFeatures.Get());
  MY_SCOPE_GUARD(resetFilter, [&]() { m_filter.SetFilter(nullptr); });

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
      if (IsStreetSynonym(m_params.GetTokens(curToken).front()))
        continue;

      if (startToken == curToken || coding::CompressedBitVector::IsEmpty(allFeatures))
        buffer = coding::CompressedBitVector::Intersect(*m_streets, *m_addressFeatures[curToken]);
      else
        buffer = coding::CompressedBitVector::Intersect(*allFeatures, *m_addressFeatures[curToken]);

      if (coding::CompressedBitVector::IsEmpty(buffer))
        break;

      allFeatures.swap(buffer);
    }

    if (coding::CompressedBitVector::IsEmpty(allFeatures))
      continue;

    if (m_filter.NeedToFilter(*allFeatures))
      allFeatures = m_filter.Filter(*allFeatures);

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

    ScopedMarkTokens mark(m_usedTokens, startToken, curToken);
    MatchPOIsAndBuildings(0 /* curToken */);
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

  // Try to consume [curToken, m_numTokens) tokens range.
  for (size_t n = 1; curToken + n <= m_numTokens && !m_usedTokens[curToken + n - 1]; ++n)
  {
    // At this point |intersection| is the intersection of
    // m_addressFeatures[curToken], m_addressFeatures[curToken + 1], ...,
    // m_addressFeatures[curToken + n - 2], iff n > 2.

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
      features = m_addressFeatures[curToken].get();
      if (m_filter.NeedToFilter(*features))
      {
        intersection = m_filter.Filter(*features);
        features = intersection.get();
      }
    }
    else
    {
      intersection =
          coding::CompressedBitVector::Intersect(*features, *m_addressFeatures[curToken + n - 1]);
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

  m_finder.ForEachReachableVertex(*m_matcher, sortedLayers,
                                  [this](IntersectionResult const & result)
  {
    ASSERT(result.IsValid(), ());
    // TODO(@y, @m, @vng): use rest fields of IntersectionResult for
    // better scoring.
    m_results->emplace_back(m_context->m_id, result.InnermostResult());
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

coding::CompressedBitVector const * Geocoder::RetrieveGeometryFeatures(MwmContext const & context,
                                                                       m2::RectD const & rect,
                                                                       int id)
{
  /// @todo
  /// - Implement more smart strategy according to id.
  /// - Move all rect limits here

  auto & features = m_geometryFeatures[context.m_id];
  for (auto const & v : features)
  {
    if (v.m_rect.IsRectInside(rect))
      return v.m_cbv.get();
  }

  auto cbv = Retrieval::RetrieveGeometryFeatures(
      context.m_value, static_cast<my::Cancellable const &>(*this), rect, m_params.m_scale);

  auto const * result = cbv.get();
  features.push_back({m2::Inflate(rect, kComparePoints, kComparePoints), move(cbv), id});
  return result;
}

bool Geocoder::AllTokensUsed() const
{
  return all_of(m_usedTokens.begin(), m_usedTokens.end(), Id<bool>());
}

bool Geocoder::HasUsedTokensInRange(size_t from, size_t to) const
{
  return any_of(m_usedTokens.begin() + from, m_usedTokens.begin() + to, Id<bool>());
}
}  // namespace v2
}  // namespace search
