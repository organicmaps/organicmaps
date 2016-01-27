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
#include "std/bind.hpp"
#include "std/iterator.hpp"
#include "std/target_os.hpp"
#include "std/transform_iterator.hpp"

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
size_t constexpr kMaxNumStates = 5;
size_t constexpr kMaxNumCountries = 5;
size_t constexpr kMaxNumLocalities = kMaxNumCities + kMaxNumStates + kMaxNumCountries;

// List of countries we're supporting search by state. Elements of the
// list should be valid prefixes of corresponding mwms names.
string const kCountriesWithStates[] = {"USA_", "Canada_"};
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

class StreetCategories
{
public:
  static StreetCategories const & Instance()
  {
    static StreetCategories const instance;
    return instance;
  }

  template <typename TFn>
  void ForEach(TFn && fn) const
  {
    for_each(m_categories.cbegin(), m_categories.cend(), forward<TFn>(fn));
  }

  bool Contains(strings::UniString const & category) const
  {
    return binary_search(m_categories.cbegin(), m_categories.cend(), category);
  }

private:
  StreetCategories()
  {
    auto const & classificator = classif();
    auto addCategory = [&](uint32_t type)
    {
      uint32_t const index = classificator.GetIndexForType(type);
      m_categories.push_back(FeatureTypeToString(index));
    };
    ftypes::IsStreetChecker::Instance().ForEachType(addCategory);
    sort(m_categories.begin(), m_categories.end());
  }

  vector<strings::UniString> m_categories;

  DISALLOW_COPY_AND_MOVE(StreetCategories);
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

bool HasSearchIndex(MwmValue const & value) { return value.m_cont.IsExist(SEARCH_INDEX_FILE_TAG); }

bool HasGeometryIndex(MwmValue & value) { return value.m_cont.IsExist(INDEX_FILE_TAG); }

MwmSet::MwmHandle FindWorld(Index & index, vector<shared_ptr<MwmInfo>> const & infos)
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

strings::UniString AsciiToUniString(char const * s)
{
  return strings::UniString(s, s + strlen(s));
}

bool IsStopWord(strings::UniString const & s)
{
  /// @todo Get all common used stop words and factor out this array into
  /// search_string_utils.cpp module for example.
  static char const * arr[] = { "a", "de", "da", "la" };

  static set<strings::UniString> const kStopWords(
      make_transform_iterator(arr, &AsciiToUniString),
      make_transform_iterator(arr + ARRAY_SIZE(arr), &AsciiToUniString));

  return kStopWords.count(s) > 0;
}

m2::RectD NormalizeViewport(m2::RectD viewport)
{
  double constexpr kMinViewportRadiusM = 5.0 * 1000;
  double constexpr kMaxViewportRadiusM = 50.0 * 1000;

  m2::RectD minViewport =
      MercatorBounds::RectByCenterXYAndSizeInMeters(viewport.Center(), kMinViewportRadiusM);
  viewport.Add(minViewport);

  m2::RectD maxViewport =
      MercatorBounds::RectByCenterXYAndSizeInMeters(viewport.Center(), kMaxViewportRadiusM);
  VERIFY(viewport.Intersect(maxViewport), ());
  return viewport;
}

m2::RectD GetRectAroundPoistion(m2::PointD const & position)
{
  double constexpr kMaxPositionRadiusM = 50.0 * 1000;
  return MercatorBounds::RectByCenterXYAndSizeInMeters(position, kMaxPositionRadiusM);
}

double GetSquaredDistance(vector<m2::RectD> const & pivots, m2::RectD const & rect)
{
  double distance = numeric_limits<double>::max();
  auto const center = rect.Center();
  for (auto const & pivot : pivots)
    distance = min(distance, center.SquareLength(pivot.Center()));
  return distance;
}

// Reorders maps in a way that prefix consists of maps intersecting
// with viewport and position, suffix consists of all other maps
// ordered by minimum distance from viewport and position.  Returns an
// iterator to the first element of the suffix.
template <typename TIt>
TIt OrderCountries(Geocoder::Params const & params, TIt begin, TIt end)
{
  vector<m2::RectD> const pivots = {NormalizeViewport(params.m_viewport),
                                    GetRectAroundPoistion(params.m_position)};
  auto compareByDistance = [&](shared_ptr<MwmInfo> const & lhs, shared_ptr<MwmInfo> const & rhs)
  {
    return GetSquaredDistance(pivots, lhs->m_limitRect) <
           GetSquaredDistance(pivots, rhs->m_limitRect);
  };
  auto intersects = [&](shared_ptr<MwmInfo> const & info) -> bool
  {
    for (auto const & pivot : pivots)
    {
      if (pivot.IsIntersect(info->m_limitRect))
        return true;
    }
    return false;
  };
  sort(begin, end, compareByDistance);
  return stable_partition(begin, end, intersects);
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
  , m_matcher(nullptr)
  , m_finder(static_cast<my::Cancellable const &>(*this))
  , m_results(nullptr)
{
}

Geocoder::~Geocoder() {}

void Geocoder::SetParams(Params const & params)
{
  m_params = params;

  // Filter stop words.
  if (m_params.m_tokens.size() > 1)
  {
    for (auto & v : m_params.m_tokens)
      v.erase(remove_if(v.begin(), v.end(), &IsStopWord), v.end());

    auto & v = m_params.m_tokens;
    v.erase(remove_if(v.begin(), v.end(), mem_fn(&Params::TSynonymsVector::empty)), v.end());

    // If all tokens are stop words - give up.
    if (m_params.m_tokens.empty())
      m_params = params;
  }

  m_retrievalParams = m_params;
  m_numTokens = m_params.m_tokens.size();
  if (!m_params.m_prefixTokens.empty())
    ++m_numTokens;

  // Remove all category synonyms for streets, as they're extracted
  // individually via LoadStreets.
  for (size_t i = 0; i < m_numTokens; ++i)
  {
    auto & synonyms = m_params.GetTokens(i);
    if (!synonyms.empty() && IsStreetSynonym(synonyms.front()))
    {
      auto b = synonyms.begin();
      auto e = synonyms.end();
      auto const & categories = StreetCategories::Instance();
      synonyms.erase(remove_if(b + 1, e, bind(&StreetCategories::Contains, cref(categories), _1)),
                     e);
    }
  }

  LOG(LDEBUG, ("Languages =", m_params.m_langs));
}

void Geocoder::GoEverywhere(vector<FeatureID> & results)
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

  vector<shared_ptr<MwmInfo>> infos;
  m_index.GetMwmsInfo(infos);

  GoImpl(infos, false /* inViewport */);
}

void Geocoder::GoInViewport(vector<FeatureID> & results)
{
  if (m_numTokens == 0)
    return;

  m_results = &results;

  vector<shared_ptr<MwmInfo>> infos;
  m_index.GetMwmsInfo(infos);

  infos.erase(remove_if(infos.begin(), infos.end(), [this](shared_ptr<MwmInfo> const & info)
  {
    return !m_params.m_viewport.IsIntersect(info->m_limitRect);
  }), infos.end());

  GoImpl(infos, true /* inViewport */);
}

void Geocoder::GoImpl(vector<shared_ptr<MwmInfo>> & infos, bool inViewport)
{
  try
  {
    // Tries to find world and fill localities table.
    {
      m_cities.clear();
      for (auto & regions : m_regions)
        regions.clear();
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

    // Orders countries by distance from viewport center and position.
    // This order is used during MatchViewportAndPosition() stage - we
    // try to match as many features as possible without trying to
    // match locality (COUNTRY or CITY), and only when there are too
    // many features, viewport and position vicinity filter is used.
    // To prevent full search in all mwms, we need to limit somehow a
    // set of mwms for MatchViewportAndPosition(), so, we always call
    // MatchViewportAndPosition() on maps intersecting with viewport
    // and on the map where the user is currently located, other maps
    // are ordered by distance from viewport and user position, and we
    // stop to call MatchViewportAndPosition() on them as soon as at
    // least one feature is found.
    size_t const numIntersectingMaps =
        distance(infos.begin(), OrderCountries(m_params, infos.begin(), infos.end()));

    // MatchViewportAndPosition() should always be matched in mwms
    // intersecting with position and viewport.
    auto const & cancellable = static_cast<my::Cancellable const&>(*this);
    auto processCountry = [&](size_t index, unique_ptr<MwmContext> context)
    {
      ASSERT(context, ());
      m_context = move(context);
      MY_SCOPE_GUARD(cleanup, [&]()
                     {
                       LOG(LDEBUG, (m_context->GetMwmName(), "processing complete."));
                       m_matcher->OnQueryFinished();
                       m_matcher = nullptr;
                       m_context.reset();
                       m_addressFeatures.clear();
                       m_streets = nullptr;
                     });

      auto it = m_matchersCache.find(m_context->m_id);
      if (it == m_matchersCache.end())
      {
        it = m_matchersCache.insert(make_pair(m_context->m_id, make_unique<FeaturesLayerMatcher>(
                    m_index, cancellable))).first;
      }
      m_matcher = it->second.get();
      m_matcher->SetContext(m_context.get());

      unique_ptr<coding::CompressedBitVector> viewportCBV;
      if (inViewport)
      {
        viewportCBV = Retrieval::RetrieveGeometryFeatures(
              m_context->m_value, cancellable,
              m_params.m_viewport, m_params.m_scale);
      }

      // Creates a cache of posting lists for each token.
      m_addressFeatures.resize(m_numTokens);
      for (size_t i = 0; i < m_numTokens; ++i)
      {
        PrepareRetrievalParams(i, i + 1);

        m_addressFeatures[i] = Retrieval::RetrieveAddressFeatures(
              m_context->m_value, cancellable, m_retrievalParams);
        ASSERT(m_addressFeatures[i], ());

        if (viewportCBV)
        {
          m_addressFeatures[i] =
              coding::CompressedBitVector::Intersect(*m_addressFeatures[i], *viewportCBV);
        }
      }

      m_streets = LoadStreets(*m_context);

      m_usedTokens.assign(m_numTokens, false);
      MatchRegions(REGION_TYPE_COUNTRY);

      if (index < numIntersectingMaps || m_results->empty())
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
  m_matchersCache.clear();
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
    // Important! Prefix match costs 1 while token match costs 2 for locality comparison.
    size_t d = 2 * (l.m_endToken - l.m_startToken);
    ASSERT_GREATER(d, 0, ());
    if (l.m_endToken == m_numTokens && !m_params.m_prefixTokens.empty())
      d -= 1;
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
    /// @todo Calculate match costs according to the exact locality name
    /// (for 'york' query "york city" is better than "new york").

    sort(preLocalities.begin(), preLocalities.end(),
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
  size_t numStates = 0;
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

#if defined(DEBUG)
        string name;
        ft.GetName(StringUtf8Multilang::DEFAULT_CODE, name);
        LOG(LDEBUG, ("City =", name));
#endif

        m_cities[make_pair(l.m_startToken, l.m_endToken)].push_back(city);
      }
      break;
    }
    case SearchModel::SEARCH_TYPE_STATE:
    {
      if (numStates < kMaxNumStates && ft.GetFeatureType() == feature::GEOM_POINT)
      {
        Region state(l, REGION_TYPE_STATE);

        string name;
        GetEnglishName(ft, name);

        for (auto const & prefix : kCountriesWithStates)
        {
          state.m_enName = prefix + name;
          strings::AsciiToLower(state.m_enName);

          state.m_ids.clear();
          m_infoGetter.GetMatchedRegions(state.m_enName, state.m_ids);
          if (!state.m_ids.empty())
          {
            LOG(LDEBUG, ("State =", state.m_enName));
            ++numStates;
            m_regions[REGION_TYPE_STATE][make_pair(l.m_startToken, l.m_endToken)].push_back(state);
          }
        }
      }
      break;
    }
    case SearchModel::SEARCH_TYPE_COUNTRY:
    {
      if (numCountries < kMaxNumCountries && ft.GetFeatureType() == feature::GEOM_POINT)
      {
        Region country(l, REGION_TYPE_COUNTRY);
        GetEnglishName(ft, country.m_enName);

        m_infoGetter.GetMatchedRegions(country.m_enName, country.m_ids);
        if (!country.m_ids.empty())
        {
          LOG(LDEBUG, ("Country =", country.m_enName));
          ++numCountries;
          m_regions[REGION_TYPE_COUNTRY][make_pair(l.m_startToken, l.m_endToken)].push_back(
              country);
        }
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
  for (size_t i = 0; i < infos.size(); ++i)
  {
    auto const & info = infos[i];
    if (info->GetType() != MwmInfo::COUNTRY)
      continue;
    auto handle = m_index.GetMwmHandleById(MwmSet::MwmId(info));
    if (!handle.IsAlive())
      continue;
    auto & value = *handle.GetValue<MwmValue>();
    if (!HasSearchIndex(value) || !HasGeometryIndex(value))
      continue;
    fn(i, make_unique<MwmContext>(move(handle)));
  }
}

void Geocoder::MatchRegions(RegionType type)
{
  switch (type)
  {
    case REGION_TYPE_STATE:
      // Tries to skip state matching and go to cities matching.
      // Then, performs states matching.
      MatchCities();
      break;
    case REGION_TYPE_COUNTRY:
      // Tries to skip country matching and go to states matching.
      // Then, performs countries matching.
      MatchRegions(REGION_TYPE_STATE);
      break;
    case REGION_TYPE_COUNT:
      ASSERT(false, ("Invalid region type."));
      return;
  }

  auto const & regions = m_regions[type];

  auto const & fileName = m_context->GetMwmName();

  // Try to match regions.
  for (auto const & p : regions)
  {
    BailIfCancelled();

    size_t const startToken = p.first.first;
    size_t const endToken = p.first.second;
    if (HasUsedTokensInRange(startToken, endToken))
      continue;

    ScopedMarkTokens mark(m_usedTokens, startToken, endToken);
    if (AllTokensUsed())
    {
      // Region matches to search query, we need to emit it as is.
      for (auto const & region : p.second)
        m_results->emplace_back(m_worldId, region.m_featureId);
      continue;
    }

    bool matches = false;
    for (auto const & region : p.second)
    {
      if (m_infoGetter.IsBelongToRegions(fileName, region.m_ids))
      {
        matches = true;
        break;
      }
    }

    if (!matches)
      continue;

    switch (type)
    {
      case REGION_TYPE_STATE:
        MatchCities();
        break;
      case REGION_TYPE_COUNTRY:
        MatchRegions(REGION_TYPE_STATE);
        break;
      case REGION_TYPE_COUNT:
        ASSERT(false, ("Invalid region type."));
        break;
    }
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

    // Filter will be applied for all non-empty bit vectors.
    LimitedSearch(allFeatures.Get(), 0);
  }
}

void Geocoder::MatchViewportAndPosition()
{
  CBVPtr allFeatures;

  // Extracts features in viewport (but not farther than some limit).
  {
    m2::RectD const rect = NormalizeViewport(m_params.m_viewport);
    allFeatures.Union(RetrieveGeometryFeatures(*m_context, rect, VIEWPORT_ID));
  }

  // Extracts features around user position.
  if (!m_params.m_viewport.IsPointInside(m_params.m_position))
  {
    m2::RectD const rect = GetRectAroundPoistion(m_params.m_position);
    allFeatures.Union(RetrieveGeometryFeatures(*m_context, rect, POSITION_ID));
  }

  // Filter will be applied only for large bit vectors.
  LimitedSearch(allFeatures.Get(), m_params.m_maxNumResults);
}

void Geocoder::LimitedSearch(coding::CompressedBitVector const * filter, size_t filterThreshold)
{
  m_filter.SetFilter(filter);
  MY_SCOPE_GUARD(resetFilter, [&]() { m_filter.SetFilter(nullptr); });

  m_filter.SetThreshold(filterThreshold);

  // The order is rather important. Match streets first, then all other stuff.
  GreedilyMatchStreets();
  MatchPOIsAndBuildings(0 /* curToken */);
}

void Geocoder::GreedilyMatchStreets()
{
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

    if (n == 1)
    {
      auto clusterize = [&](uint32_t featureId)
      {
        if (m_streets->GetBit(featureId))
          return;

        FeatureType feature;
        m_context->m_vector.GetByIndex(featureId, feature);
        feature.ParseTypes();
        SearchModel::SearchType const searchType = m_model.GetSearchType(feature);

        // All SEARCH_TYPE_CITY features were filtered in DoGeocodingWithLocalities().
        // All SEARCH_TYPE_STREET features were filtered in GreedilyMatchStreets().
        if (searchType < SearchModel::SEARCH_TYPE_STREET)
          clusters[searchType].push_back(featureId);
      };

      coding::CompressedBitVectorEnumerator::ForEach(*features, clusterize);
    }
    else
    {
      auto noFeature = [&features](uint32_t featureId) -> bool
      {
        return !features->GetBit(featureId);
      };
      for (auto & cluster : clusters)
        cluster.erase(remove_if(cluster.begin(), cluster.end(), noFeature), cluster.end());
    }

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
  StreetCategories::Instance().ForEach([&](strings::UniString const & category)
                                       {
                                         m_retrievalParams.m_tokens[0][0] = category;
                                         auto streets = Retrieval::RetrieveAddressFeatures(
                                             context.m_value, *this /* cancellable */,
                                             m_retrievalParams);
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
