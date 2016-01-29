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
#include "indexer/ftypes_matcher.hpp"
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
size_t constexpr kMaxNumVillages = 5;
size_t constexpr kMaxNumCountries = 5;

// This constant limits number of localities that will be extracted
// from World map.  Villages are not counted here as they're not
// included into World map.
size_t constexpr kMaxNumLocalities = kMaxNumCities + kMaxNumStates + kMaxNumCountries;

// List of countries we're supporting search by state. Elements of the
// list should be valid prefixes of corresponding mwms names.
string const kCountriesWithStates[] = {"US_", "Canada_"};
double constexpr kComparePoints = MercatorBounds::GetCellID2PointAbsEpsilon();

strings::UniString const kUniSpace(strings::MakeUniString(" "));

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

  vector<strings::UniString> const & GetCategories() const { return m_categories; }

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
                     strings::UniString const & sep, strings::UniString & res)
{
  ASSERT_LESS_OR_EQUAL(curToken, endToken, ());
  for (size_t i = curToken; i < endToken; ++i)
  {
    if (i < params.m_tokens.size())
    {
      res.append(params.m_tokens[i].front());
    }
    else
    {
      ASSERT_EQUAL(i, params.m_tokens.size(), ());
      res.append(params.m_prefixTokens.front());
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
      name = "us";
    else if (HasAllSubstrings(name, kUK))
      name = "uk";
    else
      return;
  }
}

// todo(@m) Refactor at least here, or even at indexer/ftypes_matcher.hpp.
vector<strings::UniString> GetVillageCategories()
{
  vector<strings::UniString> categories;

  auto const & classificator = classif();
  auto addCategory = [&](uint32_t type)
  {
    uint32_t const index = classificator.GetIndexForType(type);
    categories.push_back(FeatureTypeToString(index));
  };
  ftypes::IsVillageChecker::Instance().ForEachType(addCategory);

  return categories;
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

// Performs pairwise union of adjacent bit vectors
// until at most one bit vector is left.
void UniteCBVs(vector<unique_ptr<coding::CompressedBitVector>> & cbvs)
{
  while (cbvs.size() > 1)
  {
    size_t i = 0;
    size_t j = 0;
    for (; j + 1 < cbvs.size(); j += 2)
      cbvs[i++] = coding::CompressedBitVector::Union(*cbvs[j], *cbvs[j + 1]);
    for (; j < cbvs.size(); ++j)
      cbvs[i++] = move(cbvs[j]);
    cbvs.resize(i);
  }
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
  , m_villages(nullptr)
  , m_matcher(nullptr)
  , m_finder(static_cast<my::Cancellable const &>(*this))
  , m_lastMatchedRegion(nullptr)
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
    ASSERT(!synonyms.empty(), ());

    if (IsStreetSynonym(synonyms.front()))
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
        m_context = make_unique<MwmContext>(move(handle));
        if (HasSearchIndex(value))
        {
          PrepareAddressFeatures();
          FillLocalitiesTable();
        }
        m_context.reset();
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
                       LOG(LDEBUG, (m_context->GetName(), "geocoding complete."));
                       m_matcher->OnQueryFinished();
                       m_matcher = nullptr;
                       m_context.reset();
                       m_addressFeatures.clear();
                       m_streets = nullptr;
                       m_villages = nullptr;
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
        viewportCBV =
            Retrieval::RetrieveGeometryFeatures(m_context->m_id, m_context->m_value, cancellable,
                                                m_params.m_viewport, m_params.m_scale);
      }

      PrepareAddressFeatures();

      if (viewportCBV)
      {
        for (size_t i = 0; i < m_numTokens; ++i)
          m_addressFeatures[i] =
              coding::CompressedBitVector::Intersect(*m_addressFeatures[i], *viewportCBV);
      }

      m_streets = LoadStreets(*m_context);
      m_villages = LoadVillages(*m_context);

      auto citiesFromWorld = m_cities;
      FillVillageLocalities();
      MY_SCOPE_GUARD(remove_villages, [&]()
                     {
                       m_cities = citiesFromWorld;
                     });

      m_usedTokens.assign(m_numTokens, false);

      m_lastMatchedRegion = nullptr;
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
  m_villages.reset();
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

void Geocoder::PrepareAddressFeatures()
{
  m_addressFeatures.resize(m_numTokens);
  for (size_t i = 0; i < m_numTokens; ++i)
  {
    PrepareRetrievalParams(i, i + 1);
    m_addressFeatures[i] = Retrieval::RetrieveAddressFeatures(
        m_context->m_id, m_context->m_value, static_cast<my::Cancellable const &>(*this),
        m_retrievalParams);
    ASSERT(m_addressFeatures[i], ());
  }
}

void Geocoder::FillLocalityCandidates(coding::CompressedBitVector const * filter,
                                      size_t const maxNumLocalities,
                                      vector<Locality> & preLocalities)
{
  preLocalities.clear();

  for (size_t startToken = 0; startToken < m_numTokens; ++startToken)
  {
    CBVPtr intersection;
    intersection.SetFull();
    if (filter)
      intersection.Intersect(filter);
    intersection.Intersect(m_addressFeatures[startToken].get());
    if (intersection.IsEmpty())
      continue;

    for (size_t endToken = startToken + 1; endToken <= m_numTokens; ++endToken)
    {
      coding::CompressedBitVectorEnumerator::ForEach(*intersection.Get(),
                                                     [&](uint32_t featureId)
                                                     {
                                                       Locality l;
                                                       l.m_countryId = m_context->m_id;
                                                       l.m_featureId = featureId;
                                                       l.m_startToken = startToken;
                                                       l.m_endToken = endToken;
                                                       preLocalities.push_back(l);
                                                     });

      if (endToken < m_numTokens)
      {
        intersection.Intersect(m_addressFeatures[endToken].get());
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

  // Unique preLocalities with featureId but leave the longest range if equal.
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

  LazyRankTable rankTable(m_context->m_value);

  // Leave the most popular localities.
  if (preLocalities.size() > maxNumLocalities)
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
    preLocalities.resize(maxNumLocalities);
  }
}

void Geocoder::FillLocalitiesTable()
{
  vector<Locality> preLocalities;
  FillLocalityCandidates(nullptr, kMaxNumLocalities, preLocalities);

  size_t numCities = 0;
  size_t numStates = 0;
  size_t numCountries = 0;
  for (auto & l : preLocalities)
  {
    FeatureType ft;
    m_context->m_vector.GetByIndex(l.m_featureId, ft);

    switch (m_model.GetSearchType(ft))
    {
    case SearchModel::SEARCH_TYPE_CITY:
    {
      if (numCities < kMaxNumCities && ft.GetFeatureType() == feature::GEOM_POINT)
      {
        ++numCities;
        City city = l;
        city.m_rect = MercatorBounds::RectByCenterXYAndSizeInMeters(
            feature::GetCenter(ft), ftypes::GetRadiusByPopulation(ft.GetPopulation()));

#if defined(DEBUG)
        string name;
        ft.GetName(StringUtf8Multilang::DEFAULT_CODE, name);
        LOG(LDEBUG, ("City =", name));
#endif

        m_cities[{l.m_startToken, l.m_endToken}].push_back(city);
      }
      break;
    }
    case SearchModel::SEARCH_TYPE_STATE:
    {
      if (numStates < kMaxNumStates && ft.GetFeatureType() == feature::GEOM_POINT)
      {
        Region state(l, REGION_TYPE_STATE);
        state.m_center = ft.GetCenter();

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
        country.m_center = ft.GetCenter();

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

void Geocoder::FillVillageLocalities()
{
  vector<Locality> preLocalities;
  FillLocalityCandidates(m_villages.get(), kMaxNumVillages, preLocalities);

  size_t numVillages = 0;

  for (auto & l : preLocalities)
  {
    FeatureType ft;
    m_context->m_vector.GetByIndex(l.m_featureId, ft);

    if (m_model.GetSearchType(ft) != SearchModel::SEARCH_TYPE_VILLAGE)
      continue;
    if (ft.GetFeatureType() != feature::GEOM_POINT)
      continue;
    if (numVillages >= kMaxNumVillages)
      continue;

    ++numVillages;
    City village = l;
    village.m_rect = MercatorBounds::RectByCenterXYAndSizeInMeters(
        feature::GetCenter(ft), ftypes::GetRadiusByPopulation(ft.GetPopulation()));

#if defined(DEBUG)
    string name;
    ft.GetName(StringUtf8Multilang::DEFAULT_CODE, name);
    LOG(LDEBUG, ("Village =", name));
#endif

    m_cities[{l.m_startToken, l.m_endToken}].push_back(village);
  }
}

template <typename TFn>
void Geocoder::ForEachCountry(vector<shared_ptr<MwmInfo>> const & infos, TFn && fn)
{
  for (size_t i = 0; i < infos.size(); ++i)
  {
    auto const & info = infos[i];
    if (info->GetType() != MwmInfo::COUNTRY && info->GetType() != MwmInfo::WORLD)
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

  auto const & fileName = m_context->GetName();
  bool const isWorld = m_context->GetInfo()->GetType() == MwmInfo::WORLD;

  // Try to match regions.
  for (auto const & p : regions)
  {
    BailIfCancelled();

    size_t const startToken = p.first.first;
    size_t const endToken = p.first.second;
    if (HasUsedTokensInRange(startToken, endToken))
      continue;

    for (auto const & region : p.second)
    {
      bool matches = false;

      // On the World.mwm we need to check that CITY - STATE - COUNTRY
      // form a nested sequence.  Otherwise, as mwm borders do not
      // intersect state or country boundaries, it's enough to check
      // mwm that is currently being processed belongs to region.
      if (isWorld)
      {
        matches = m_lastMatchedRegion == nullptr ||
                  m_infoGetter.IsBelongToRegions(region.m_center, m_lastMatchedRegion->m_ids);
      }
      else
      {
        matches = m_infoGetter.IsBelongToRegions(fileName, region.m_ids);
      }

      if (!matches)
        continue;

      ScopedMarkTokens mark(m_usedTokens, startToken, endToken);
      if (AllTokensUsed())
      {
        // Region matches to search query, we need to emit it as is.
        m_results->emplace_back(m_worldId, region.m_featureId);
        continue;
      }

      m_lastMatchedRegion = &region;
      MY_SCOPE_GUARD(cleanup, [this]() { m_lastMatchedRegion = nullptr; });
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
}

void Geocoder::MatchCities()
{
  // Localities are ordered my (m_startToken, m_endToken) pairs.
  for (auto const & p : m_cities)
  {
    size_t const startToken = p.first.first;
    size_t const endToken = p.first.second;
    if (HasUsedTokensInRange(startToken, endToken))
      continue;

    for (auto const & city : p.second)
    {
      BailIfCancelled();

      if (m_lastMatchedRegion &&
          !m_infoGetter.IsBelongToRegions(city.m_rect.Center(), m_lastMatchedRegion->m_ids))
      {
        continue;
      }

      ScopedMarkTokens mark(m_usedTokens, startToken, endToken);
      if (AllTokensUsed())
      {
        // City matches to search query.
        m_results->emplace_back(city.m_countryId, city.m_featureId);
        continue;
      }

      // No need to search features in the World map.
      if (m_context->GetInfo()->GetType() == MwmInfo::WORLD)
        continue;

      auto const * cityFeatures = RetrieveGeometryFeatures(*m_context, city.m_rect, city.m_featureId);

      if (coding::CompressedBitVector::IsEmpty(cityFeatures))
        continue;

      // Filter will be applied for all non-empty bit vectors.
      LimitedSearch(cityFeatures, 0 /* filterThreshold */);
    }
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
  LimitedSearch(allFeatures.Get(), m_params.m_maxNumResults /* filterThreshold */);
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
  for (size_t startToken = 0; startToken < m_numTokens; ++startToken)
  {
    if (m_usedTokens[startToken])
      continue;

    // Here we try to match as many tokens as possible while
    // intersection is a non-empty bit vector of streets.  All tokens
    // that are synonyms to streets are ignored.  Moreover, each time
    // a token that looks like a beginning of a house number is met,
    // we try to use current intersection of tokens as a street layer
    // and try to match buildings or pois.
    unique_ptr<coding::CompressedBitVector> allFeatures;

    size_t curToken = startToken;
    for (; curToken < m_numTokens && !m_usedTokens[curToken]; ++curToken)
    {
      auto const & token = m_params.GetTokens(curToken).front();
      if (IsStreetSynonym(token))
        continue;

      if (feature::IsHouseNumber(token) &&
          !coding::CompressedBitVector::IsEmpty(allFeatures))
      {
        CreateStreetsLayerAndMatchLowerLayers(startToken, curToken, allFeatures);
      }

      unique_ptr<coding::CompressedBitVector> buffer;
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

    CreateStreetsLayerAndMatchLowerLayers(startToken, curToken, allFeatures);
  }
}

void Geocoder::CreateStreetsLayerAndMatchLowerLayers(
    size_t startToken, size_t endToken, unique_ptr<coding::CompressedBitVector> const & features)
{
  ASSERT(m_layers.empty(), ());

  if (coding::CompressedBitVector::IsEmpty(features))
    return;

  CBVPtr filtered(features.get(), false /* isOwner */);
  if (m_filter.NeedToFilter(*features))
    filtered.Set(m_filter.Filter(*features).release(), true /* isOwner */);

  m_layers.emplace_back();
  MY_SCOPE_GUARD(cleanupGuard, bind(&vector<FeaturesLayer>::pop_back, &m_layers));

  auto & layer = m_layers.back();
  layer.Clear();
  layer.m_type = SearchModel::SEARCH_TYPE_STREET;
  layer.m_startToken = startToken;
  layer.m_endToken = endToken;
  JoinQueryTokens(m_params, layer.m_startToken, layer.m_endToken, kUniSpace /* sep */,
                  layer.m_subQuery);

  vector<uint32_t> sortedFeatures;
  sortedFeatures.reserve(features->PopCount());
  coding::CompressedBitVectorEnumerator::ForEach(*filtered, MakeBackInsertFunctor(sortedFeatures));
  layer.m_sortedFeatures = &sortedFeatures;

  ScopedMarkTokens mark(m_usedTokens, startToken, endToken);
  MatchPOIsAndBuildings(0 /* curToken */);
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
      JoinQueryTokens(m_params, layer.m_startToken, layer.m_endToken, kUniSpace /* sep */,
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

        // All SEARCH_TYPE_CITY features were filtered in MatchCities().
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
      // MatchPOIsAndBuildings().  This may lead to use-after-free.
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

  // Layers ordered by search type.
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

unique_ptr<coding::CompressedBitVector> Geocoder::LoadCategories(
    MwmContext & context, vector<strings::UniString> const & categories)
{
  ASSERT(context.m_handle.IsAlive(), ());
  ASSERT(HasSearchIndex(context.m_value), ());

  m_retrievalParams.m_tokens.resize(1);
  m_retrievalParams.m_tokens[0].resize(1);
  m_retrievalParams.m_prefixTokens.clear();

  vector<unique_ptr<coding::CompressedBitVector>> cbvs;

  for_each(categories.begin(), categories.end(), [&](strings::UniString const & category)
           {
             m_retrievalParams.m_tokens[0][0] = category;
             auto cbv = Retrieval::RetrieveAddressFeatures(
                 context.m_id, context.m_value, static_cast<my::Cancellable const &>(*this),
                 m_retrievalParams);
             if (!coding::CompressedBitVector::IsEmpty(cbv))
               cbvs.push_back(move(cbv));
           });

  UniteCBVs(cbvs);
  if (cbvs.empty())
    cbvs.push_back(make_unique<coding::DenseCBV>());

  return move(cbvs[0]);
}

coding::CompressedBitVector const * Geocoder::LoadStreets(MwmContext & context)
{
  if (!context.m_handle.IsAlive() || !HasSearchIndex(context.m_value))
    return nullptr;

  auto mwmId = context.m_handle.GetId();
  auto const it = m_streetsCache.find(mwmId);
  if (it != m_streetsCache.cend())
    return it->second.get();

  auto streets = LoadCategories(context, StreetCategories::Instance().GetCategories());

  auto const * result = streets.get();
  m_streetsCache[mwmId] = move(streets);
  return result;
}

unique_ptr<coding::CompressedBitVector> Geocoder::LoadVillages(MwmContext & context)
{
  if (!context.m_handle.IsAlive() || !HasSearchIndex(context.m_value))
    return make_unique<coding::DenseCBV>();

  return LoadCategories(context, GetVillageCategories());
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

  auto cbv = Retrieval::RetrieveGeometryFeatures(context.m_id, context.m_value,
                                                 static_cast<my::Cancellable const &>(*this), rect,
                                                 m_params.m_scale);

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
