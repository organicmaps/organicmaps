#include "search/v2/geocoder.hpp"

#include "search/dummy_rank_table.hpp"
#include "search/retrieval.hpp"
#include "search/search_query.hpp"
#include "search/v2/cbv_ptr.hpp"
#include "search/v2/features_filter.hpp"
#include "search/v2/features_layer_matcher.hpp"
#include "search/v2/locality_scorer.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"
#include "indexer/rank_table.hpp"
#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

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
#include "std/sstream.hpp"
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
// @vng Set this value to possible maximum.
size_t const kMaxNumLocalities = LocalityScorer::kDefaultReadLimit;

size_t constexpr kPivotRectsCacheSize = 10;
size_t constexpr kLocalityRectsCacheSize = 10;

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

class LazyRankTable : public RankTable
{
 public:
  LazyRankTable(MwmValue const & value) : m_value(value) {}

  uint8_t Get(uint64_t i) const override
  {
    EnsureTableLoaded();
    return m_table->Get(i);
  }

  uint64_t Size() const override
  {
    EnsureTableLoaded();
    return m_table->Size();
  }

  RankTable::Version GetVersion() const override
  {
    EnsureTableLoaded();
    return m_table->GetVersion();
  }

  void Serialize(Writer & writer, bool preserveHostEndiannes) override
  {
    EnsureTableLoaded();
    m_table->Serialize(writer, preserveHostEndiannes);
  }

 private:
   void EnsureTableLoaded() const
   {
     if (m_table)
       return;
     m_table = search::RankTable::Load(m_value.m_cont);
     if (!m_table)
       m_table = make_unique<search::DummyRankTable>();
   }

  MwmValue const & m_value;
  mutable unique_ptr<search::RankTable> m_table;
};

class LocalityScorerDelegate : public LocalityScorer::Delegate
{
public:
  LocalityScorerDelegate(MwmContext const & context, Geocoder::Params const & params)
    : m_context(context), m_params(params), m_ranks(m_context.m_value)
  {
  }

  // LocalityScorer::Delegate overrides:
  void GetNames(uint32_t featureId, vector<string> & names) const override
  {
    FeatureType ft;
    if (!m_context.GetFeature(featureId, ft))
      return;
    for (auto const & lang : m_params.m_langs)
    {
      string name;
      if (ft.GetName(lang, name))
        names.push_back(name);
    }
  }

  uint8_t GetRank(uint32_t featureId) const override { return m_ranks.Get(featureId); }

private:
  MwmContext const & m_context;
  Geocoder::Params const & m_params;
  LazyRankTable m_ranks;
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

void GetAffiliationName(FeatureType const & ft, string & name)
{
  VERIFY(ft.GetName(StringUtf8Multilang::kDefaultCode, name), ());
  ASSERT(!name.empty(), ());
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

double Area(m2::RectD const & rect)
{
  return rect.IsValid() ? rect.SizeX() * rect.SizeY() : 0;
}

// Computes an average similaty between |rect| and |pivot|. By
// similarity between two rects we mean a fraction of the area of
// rects intersection to the area of the smallest rect.
double GetSimilarity(m2::RectD const & pivot, m2::RectD const & rect)
{
  double const area = min(Area(pivot), Area(rect));
  if (area == 0.0)
    return 0.0;
  m2::RectD p = pivot;
  if (!p.Intersect(rect))
    return 0.0;
  return Area(p) / area;
}

// Returns shortest squared distance from the center of |pivot| to the
// center of |rect|.
double GetSquaredDistance(m2::RectD const & pivot, m2::RectD const & rect)
{
  auto const center = rect.Center();
  return center.SquareLength(pivot.Center());
}

struct KeyedMwmInfo
{
  KeyedMwmInfo(shared_ptr<MwmInfo> const & info, m2::RectD const & pivot) : m_info(info)
  {
    auto const & rect = m_info->m_limitRect;
    m_similarity = GetSimilarity(pivot, rect);
    m_distance = GetSquaredDistance(pivot, rect);
  }

  bool operator<(KeyedMwmInfo const & rhs) const
  {
    if (m_similarity != rhs.m_similarity)
      return m_similarity > rhs.m_similarity;
    return m_distance < rhs.m_distance;
  }

  shared_ptr<MwmInfo> m_info;
  double m_similarity;
  double m_distance;
};

// Reorders maps in a way that prefix consists of maps intersecting
// with pivot, suffix consists of all other maps ordered by minimum
// distance from pivot. Returns number of maps in prefix.
size_t OrderCountries(m2::RectD const & pivot, vector<shared_ptr<MwmInfo>> & infos)
{
  // TODO (@y): remove this if crashes in this function
  // disappear. Otherwise, remove null infos and re-check MwmSet
  // again.
  for (auto const & info : infos)
  {
    CHECK(info.get(),
          ("MwmSet invariant violated. Please, contact @y if you know how to reproduce this."));
  }

  vector<KeyedMwmInfo> keyedInfos;
  keyedInfos.reserve(infos.size());
  for (auto const & info : infos)
    keyedInfos.emplace_back(info, pivot);
  sort(keyedInfos.begin(), keyedInfos.end());

  infos.clear();
  for (auto const & info : keyedInfos)
    infos.emplace_back(info.m_info);

  auto intersects = [&](shared_ptr<MwmInfo> const & info) -> bool
  {
    return pivot.IsIntersect(info->m_limitRect);
  };

  auto const sep = stable_partition(infos.begin(), infos.end(), intersects);
  return distance(infos.begin(), sep);
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

bool SameMwm(Geocoder::TResult const & lhs, Geocoder::TResult const & rhs)
{
  return lhs.first.m_mwmId == rhs.first.m_mwmId;
}
}  // namespace

// Geocoder::Params --------------------------------------------------------------------------------
Geocoder::Params::Params()
  : m_mode(Mode::Everywhere), m_accuratePivotCenter(0, 0), m_maxNumResults(0)
{
}

// Geocoder::Geocoder ------------------------------------------------------------------------------
Geocoder::Geocoder(Index & index, storage::CountryInfoGetter const & infoGetter)
  : m_index(index)
  , m_infoGetter(infoGetter)
  , m_numTokens(0)
  , m_model(SearchModel::Instance())
  , m_pivotRectsCache(kPivotRectsCacheSize, static_cast<my::Cancellable const &>(*this),
                      Query::kMaxViewportRadiusM)
  , m_localityRectsCache(kLocalityRectsCacheSize, static_cast<my::Cancellable const &>(*this))
  , m_pivotFeatures(index)
  , m_streets(nullptr)
  , m_villages(nullptr)
  , m_filter(nullptr)
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
      my::EraseIf(v, &IsStopWord);

    auto & v = m_params.m_tokens;
    my::EraseIf(v, mem_fn(&Params::TSynonymsVector::empty));

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

void Geocoder::GoEverywhere(TResultList & results)
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

void Geocoder::GoInViewport(TResultList & results)
{
  if (m_numTokens == 0)
    return;

  m_results = &results;

  vector<shared_ptr<MwmInfo>> infos;
  m_index.GetMwmsInfo(infos);

  my::EraseIf(infos, [this](shared_ptr<MwmInfo> const & info)
  {
    return !m_params.m_pivot.IsIntersect(info->m_limitRect);
  });

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
    // This order is used during MatchAroundPivot() stage - we try to
    // match as many features as possible without trying to match
    // locality (COUNTRY or CITY), and only when there are too many
    // features, viewport and position vicinity filter is used.  To
    // prevent full search in all mwms, we need to limit somehow a set
    // of mwms for MatchAroundPivot(), so, we always call
    // MatchAroundPivot() on maps intersecting with pivot rect, other
    // maps are ordered by distance from pivot, and we stop to call
    // MatchAroundPivot() on them as soon as at least one feature is
    // found.
    size_t const numIntersectingMaps = OrderCountries(m_params.m_pivot, infos);

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

      auto it = m_matchersCache.find(m_context->GetId());
      if (it == m_matchersCache.end())
      {
        it = m_matchersCache.insert(make_pair(m_context->GetId(), make_unique<FeaturesLayerMatcher>(
                    m_index, cancellable))).first;
      }
      m_matcher = it->second.get();
      m_matcher->SetContext(m_context.get());

      PrepareAddressFeatures();

      coding::CompressedBitVector const * viewportCBV = nullptr;
      if (inViewport)
        viewportCBV = RetrieveGeometryFeatures(*m_context, m_params.m_pivot, RECT_ID_PIVOT);

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
        MatchAroundPivot();
    };

    // Iterates through all alive mwms and performs geocoding.
    ForEachCountry(infos, processCountry);
  }
  catch (CancelException & e)
  {
  }

  // Fill results ranks, as they were missed.
  FillMissingFieldsInResults();
}

void Geocoder::ClearCaches()
{
  m_pivotRectsCache.Clear();
  m_localityRectsCache.Clear();
  m_pivotFeatures.Clear();

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
    m_addressFeatures[i] = RetrieveAddressFeatures(
        m_context->GetId(), m_context->m_value, static_cast<my::Cancellable const &>(*this),
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
      // Skip locality candidates that match only numbers.
      if (!m_params.IsNumberTokens(startToken, endToken))
      {
        intersection.ForEach([&](uint32_t featureId)
                             {
                               Locality l;
                               l.m_countryId = m_context->GetId();
                               l.m_featureId = featureId;
                               l.m_startToken = startToken;
                               l.m_endToken = endToken;
                               preLocalities.push_back(l);
                             });
      }

      if (endToken < m_numTokens)
      {
        intersection.Intersect(m_addressFeatures[endToken].get());
        if (intersection.IsEmpty())
          break;
      }
    }
  }

  LocalityScorerDelegate delegate(*m_context, m_params);
  LocalityScorer scorer(m_params, delegate);
  scorer.GetTopLocalities(maxNumLocalities, preLocalities);
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
    m_context->GetFeature(l.m_featureId, ft);

    auto addRegionMaps = [&](size_t & count, size_t maxCount, RegionType type)
    {
      if (count < maxCount && ft.GetFeatureType() == feature::GEOM_POINT)
      {
        Region region(l, type);
        region.m_center = ft.GetCenter();

        string name;
        GetAffiliationName(ft, region.m_enName);
        LOG(LDEBUG, ("Region =", region.m_enName));

        m_infoGetter.GetMatchedRegions(region.m_enName, region.m_ids);
        if (region.m_ids.empty())
          LOG(LWARNING, ("Maps not found for region", region.m_enName));

        ++count;
        m_regions[type][make_pair(l.m_startToken, l.m_endToken)].push_back(region);
      }
    };

    switch (m_model.GetSearchType(ft))
    {
    case SearchModel::SEARCH_TYPE_CITY:
    {
      if (numCities < kMaxNumCities && ft.GetFeatureType() == feature::GEOM_POINT)
      {
        ++numCities;

        auto const center = feature::GetCenter(ft);
        auto const population = ft.GetPopulation();
        auto const radius = ftypes::GetRadiusByPopulation(population);

        City city(l, SearchModel::SEARCH_TYPE_CITY);
        city.m_rect = MercatorBounds::RectByCenterXYAndSizeInMeters(center, radius);

#if defined(DEBUG)
        ft.GetName(StringUtf8Multilang::kDefaultCode, city.m_defaultName);
        LOG(LDEBUG, ("City =", city.m_defaultName, radius));
#endif

        m_cities[{l.m_startToken, l.m_endToken}].push_back(city);
      }
      break;
    }
    case SearchModel::SEARCH_TYPE_STATE:
    {
      addRegionMaps(numStates, kMaxNumStates, REGION_TYPE_STATE);
      break;
    }
    case SearchModel::SEARCH_TYPE_COUNTRY:
    {
      addRegionMaps(numCountries, kMaxNumCountries, REGION_TYPE_COUNTRY);
      break;
    }
    default:
      break;
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
    m_context->GetFeature(l.m_featureId, ft);

    if (m_model.GetSearchType(ft) != SearchModel::SEARCH_TYPE_VILLAGE)
      continue;

    // We accept lines and areas as village features.
    auto const center = feature::GetCenter(ft);
    ++numVillages;
    City village(l, SearchModel::SEARCH_TYPE_VILLAGE);

    auto const population = ft.GetPopulation();
    double const radius = ftypes::GetRadiusByPopulation(population);
    village.m_rect = MercatorBounds::RectByCenterXYAndSizeInMeters(center, radius);

#if defined(DEBUG)
    ft.GetName(StringUtf8Multilang::kDefaultCode, village.m_defaultName);
    LOG(LDEBUG, ("Village =", village.m_defaultName));
#endif

    m_cities[{l.m_startToken, l.m_endToken}].push_back(village);
    if (numVillages >= kMaxNumVillages)
      break;
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
    if (info->GetType() == MwmInfo::COUNTRY && m_params.m_mode == Mode::World)
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
        EmitResult(region, startToken, endToken);
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
        // City matches to search query, we need to emit it as is.
        EmitResult(city, startToken, endToken);
        continue;
      }

      // No need to search features in the World map.
      if (m_context->GetInfo()->GetType() == MwmInfo::WORLD)
        continue;

      auto const * cityFeatures =
          RetrieveGeometryFeatures(*m_context, city.m_rect, RECT_ID_LOCALITY);

      if (coding::CompressedBitVector::IsEmpty(cityFeatures))
        continue;

      LocalityFilter filter(*cityFeatures);
      LimitedSearch(filter);
    }
  }
}

void Geocoder::MatchAroundPivot()
{
  auto const * features = RetrieveGeometryFeatures(*m_context, m_params.m_pivot, RECT_ID_PIVOT);

  if (!features)
    return;

  ViewportFilter filter(*features, m_params.m_maxNumResults /* threshold */);
  LimitedSearch(filter);
}

void Geocoder::LimitedSearch(FeaturesFilter const & filter)
{
  m_filter = &filter;
  MY_SCOPE_GUARD(resetFilter, [&]() { m_filter = nullptr; });

  // The order is rather important. Match streets first, then all other stuff.
  GreedilyMatchStreets();
  MatchPOIsAndBuildings(0 /* curToken */);
  MatchUnclassified(0 /* curToken */);
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

    // This variable is used for prevention of duplicate calls to
    // CreateStreetsLayerAndMatchLowerLayers() with the same
    // arguments.
    size_t lastStopToken = curToken;

    for (; curToken < m_numTokens && !m_usedTokens[curToken]; ++curToken)
    {
      auto const & token = m_params.GetTokens(curToken).front();
      if (IsStreetSynonymPrefix(token))
        continue;

      if (feature::IsHouseNumber(token))
      {
        CreateStreetsLayerAndMatchLowerLayers(startToken, curToken, allFeatures);
        lastStopToken = curToken;
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

    if (curToken != lastStopToken)
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
  if (m_filter->NeedToFilter(*features))
    filtered.Set(m_filter->Filter(*features).release(), true /* isOwner */);

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
  filtered.ForEach(MakeBackInsertFunctor(sortedFeatures));
  layer.m_sortedFeatures = &sortedFeatures;

  ScopedMarkTokens mark(m_usedTokens, startToken, endToken);
  MatchPOIsAndBuildings(0 /* curToken */);
}

void Geocoder::MatchPOIsAndBuildings(size_t curToken)
{
  BailIfCancelled();

  curToken = SkipUsedTokens(curToken);
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
  size_t const kNumClusters = SearchModel::SEARCH_TYPE_STREET;
  vector<uint32_t> clusters[kNumClusters];

  // Appends |featureId| to the end of the corresponding cluster, if
  // any.
  auto clusterize = [&](uint32_t featureId)
  {
    auto const searchType = GetSearchTypeInGeocoding(featureId);

    // All SEARCH_TYPE_CITY features were filtered in
    // MatchCities().  All SEARCH_TYPE_STREET features were
    // filtered in GreedilyMatchStreets().
    if (searchType < SearchModel::SEARCH_TYPE_STREET)
      clusters[searchType].push_back(featureId);
  };

  CBVPtr features;
  features.SetFull();

  // Try to consume [curToken, m_numTokens) tokens range.
  for (size_t n = 1; curToken + n <= m_numTokens && !m_usedTokens[curToken + n - 1]; ++n)
  {
    // At this point |features| is the intersection of
    // m_addressFeatures[curToken], m_addressFeatures[curToken + 1],
    // ..., m_addressFeatures[curToken + n - 2].

    BailIfCancelled();

    {
      auto & layer = m_layers.back();
      layer.Clear();
      layer.m_startToken = curToken;
      layer.m_endToken = curToken + n;
      JoinQueryTokens(m_params, layer.m_startToken, layer.m_endToken, kUniSpace /* sep */,
                      layer.m_subQuery);
    }

    features.Intersect(m_addressFeatures[curToken + n - 1].get());
    ASSERT(features.Get(), ());

    CBVPtr filtered;
    if (m_filter->NeedToFilter(*features))
      filtered.Set(m_filter->Filter(*features));
    else
      filtered.Set(features.Get(), false /* isOwner */);
    ASSERT(filtered.Get(), ());

    bool const looksLikeHouseNumber = feature::IsHouseNumber(m_layers.back().m_subQuery);

    if (filtered.IsEmpty() && !looksLikeHouseNumber)
      break;

    if (n == 1)
    {
      filtered.ForEach(clusterize);
    }
    else
    {
      auto noFeature = [&filtered](uint32_t featureId) -> bool
      {
        return !filtered->GetBit(featureId);
      };
      for (auto & cluster : clusters)
        my::EraseIf(cluster, noFeature);

      size_t curs[kNumClusters] = {};
      size_t ends[kNumClusters];
      for (size_t i = 0; i < kNumClusters; ++i)
        ends[i] = clusters[i].size();
      filtered.ForEach([&](uint32_t featureId)
      {
        bool found = false;
        for (size_t i = 0; i < kNumClusters && !found; ++i)
        {
          size_t & cur = curs[i];
          size_t const end = ends[i];
          while (cur != end && clusters[i][cur] < featureId)
            ++cur;
          if (cur != end && clusters[i][cur] == featureId)
            found = true;
        }
        if (!found)
          clusterize(featureId);
      });
      for (size_t i = 0; i < kNumClusters; ++i)
        inplace_merge(clusters[i].begin(), clusters[i].begin() + ends[i], clusters[i].end());
    }

    for (size_t i = 0; i < kNumClusters; ++i)
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
    else if (layer.m_type == SearchModel::SEARCH_TYPE_STREET)
      streetIndex = i;
  }

  bool const hasBuildings = buildingIndex != m_layers.size();
  bool const hasStreets = streetIndex != m_layers.size();

  // Checks that building and street layers are neighbours.
  if (hasBuildings && hasStreets)
  {
    auto const & buildings = m_layers[buildingIndex];
    auto const & streets = m_layers[streetIndex];
    if (buildings.m_startToken != streets.m_endToken &&
        buildings.m_endToken != streets.m_startToken)
    {
      return false;
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

  auto const & innermostLayer = *sortedLayers.front();

  m_finder.ForEachReachableVertex(*m_matcher, sortedLayers,
                                  [this, &innermostLayer](IntersectionResult const & result)
  {
    ASSERT(result.IsValid(), ());
    // TODO(@y, @m, @vng): use rest fields of IntersectionResult for
    // better scoring.
    EmitResult(m_context->GetId(), result.InnermostResult(), innermostLayer.m_type,
               innermostLayer.m_startToken, innermostLayer.m_endToken);
  });
}

void Geocoder::EmitResult(MwmSet::MwmId const & mwmId, uint32_t ftId, SearchModel::SearchType type,
                          size_t startToken, size_t endToken)
{
  FeatureID id(mwmId, ftId);

  PreRankingInfo info;
  info.m_searchType = type;
  info.m_startToken = startToken;
  info.m_endToken = endToken;

  // Other fields will be filled at the end, for all results at once.
  m_results->emplace_back(move(id), move(info));
}

void Geocoder::EmitResult(Region const & region, size_t startToken, size_t endToken)
{
  SearchModel::SearchType type;
  switch (region.m_type)
  {
  case REGION_TYPE_STATE: type = SearchModel::SEARCH_TYPE_STATE; break;
  case REGION_TYPE_COUNTRY: type = SearchModel::SEARCH_TYPE_COUNTRY; break;
  case REGION_TYPE_COUNT: type = SearchModel::SEARCH_TYPE_COUNT; break;
  }
  EmitResult(m_worldId, region.m_featureId, type, startToken, endToken);
}

void Geocoder::EmitResult(City const & city, size_t startToken, size_t endToken)
{
  EmitResult(city.m_countryId, city.m_featureId, city.m_type, startToken, endToken);
}

void Geocoder::FillMissingFieldsInResults()
{
  sort(m_results->begin(), m_results->end(), my::CompareBy(&TResult::first));

  auto ib = m_results->begin();
  while (ib != m_results->end())
  {
    auto ie = ib;
    while (ie != m_results->end() && SameMwm(*ib, *ie))
      ++ie;

    /// @todo Add RankTableCache here?
    MwmSet::MwmHandle handle = m_index.GetMwmHandleById(ib->first.m_mwmId);
    if (handle.IsAlive())
    {
      auto rankTable = RankTable::Load(handle.GetValue<MwmValue>()->m_cont);
      if (!rankTable.get())
        rankTable.reset(new DummyRankTable());

      for (auto ii = ib; ii != ie; ++ii)
      {
        auto const & id = ii->first;
        auto & info = ii->second;

        info.m_rank = rankTable->Get(id.m_index);
      }
    }
    ib = ie;
  }

  if (m_results->size() > m_params.m_maxNumResults)
  {
    m_pivotFeatures.SetPosition(m_params.m_accuratePivotCenter, m_params.m_scale);
    for (auto & result : *m_results)
    {
      auto const & id = result.first;
      auto & info = result.second;
      info.m_distanceToPivot = m_pivotFeatures.GetDistanceToFeatureMeters(id);
    }
  }
}

void Geocoder::MatchUnclassified(size_t curToken)
{
  ASSERT(m_layers.empty(), ());

  // We need to match all unused tokens to UNCLASSIFIED features,
  // therefore unused tokens must be adjacent to each other.  For
  // example, as parks are UNCLASSIFIED now, it's ok to match "London
  // Hyde Park", because London will be matched as a city and rest
  // adjacent tokens will be matched to "Hyde Park", whereas it's not
  // ok to match something to "Park London Hyde", because tokens
  // "Park" and "Hyde" are not adjacent.
  if (NumUnusedTokensGroups() != 1)
    return;

  CBVPtr allFeatures;
  allFeatures.SetFull();

  auto startToken = curToken;
  for (curToken = SkipUsedTokens(curToken); curToken < m_numTokens && !m_usedTokens[curToken];
       ++curToken)
  {
    allFeatures.Intersect(m_addressFeatures[curToken].get());
  }

  if (m_filter->NeedToFilter(*allFeatures))
    allFeatures.Set(m_filter->Filter(*allFeatures).release(), true /* isOwner */);

  if (allFeatures.IsEmpty())
    return;

  auto emitUnclassified = [&](uint32_t featureId)
  {
    auto type = GetSearchTypeInGeocoding(featureId);
    if (type == SearchModel::SEARCH_TYPE_UNCLASSIFIED)
      EmitResult(m_context->GetId(), featureId, type, startToken, curToken);
  };
  allFeatures.ForEach(emitUnclassified);
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
             auto cbv = RetrieveAddressFeatures(
                 context.GetId(), context.m_value, static_cast<my::Cancellable const &>(*this),
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
                                                                       RectId id)
{
  switch (id)
  {
  case RECT_ID_PIVOT: return m_pivotRectsCache.Get(context, rect, m_params.m_scale);
  case RECT_ID_LOCALITY: return m_localityRectsCache.Get(context, rect, m_params.m_scale);
  case RECT_ID_COUNT: ASSERT(false, ("Invalid RectId.")); return nullptr;
  }
}

SearchModel::SearchType Geocoder::GetSearchTypeInGeocoding(uint32_t featureId)
{
  if (m_streets->GetBit(featureId))
    return SearchModel::SEARCH_TYPE_STREET;
  if (m_villages->GetBit(featureId))
    return SearchModel::SEARCH_TYPE_VILLAGE;

  FeatureType feature;
  m_context->GetFeature(featureId, feature);
  return m_model.GetSearchType(feature);
}

bool Geocoder::AllTokensUsed() const
{
  return all_of(m_usedTokens.begin(), m_usedTokens.end(), Id<bool>());
}

bool Geocoder::HasUsedTokensInRange(size_t from, size_t to) const
{
  return any_of(m_usedTokens.begin() + from, m_usedTokens.begin() + to, Id<bool>());
}

size_t Geocoder::NumUnusedTokensGroups() const
{
  size_t numGroups = 0;
  for (size_t i = 0; i < m_usedTokens.size(); ++i)
  {
    if (!m_usedTokens[i] && (i == 0 || m_usedTokens[i - 1]))
      ++numGroups;
  }
  return numGroups;
}

size_t Geocoder::SkipUsedTokens(size_t curToken) const
{
  while (curToken != m_usedTokens.size() && m_usedTokens[curToken])
    ++curToken;
  return curToken;
}

string DebugPrint(Geocoder::Locality const & locality)
{
  ostringstream os;
  os << "Locality [" << DebugPrint(locality.m_countryId) << ", featureId=" << locality.m_featureId
     << ", startToken=" << locality.m_startToken << ", endToken=" << locality.m_endToken << "]";
  return os.str();
}
}  // namespace v2
}  // namespace search
