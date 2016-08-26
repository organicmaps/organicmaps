#include "search/geocoder.hpp"

#include "search/cbv.hpp"
#include "search/dummy_rank_table.hpp"
#include "search/features_filter.hpp"
#include "search/features_layer_matcher.hpp"
#include "search/house_numbers_matcher.hpp"
#include "search/locality_scorer.hpp"
#include "search/pre_ranker.hpp"
#include "search/processor.hpp"
#include "search/retrieval.hpp"
#include "search/token_slice.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"
#include "indexer/postcodes_matcher.hpp"
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

struct ScopedMarkTokens
{
  ScopedMarkTokens(vector<bool> & usedTokens, size_t from, size_t to)
    : m_usedTokens(usedTokens), m_from(from), m_to(to)
  {
    ASSERT_LESS_OR_EQUAL(m_from, m_to, ());
    ASSERT_LESS_OR_EQUAL(m_to, m_usedTokens.size(), ());
#if defined(DEBUG)
    for (size_t i = m_from; i != m_to; ++i)
      ASSERT(!m_usedTokens[i], (i));
#endif
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

void JoinQueryTokens(QueryParams const & params, size_t curToken, size_t endToken,
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
      CHECK_EQUAL(i, params.m_tokens.size(), ());
      CHECK(!params.m_prefixTokens.empty(), ());
      res.append(params.m_prefixTokens.front());
    }

    if (i + 1 != endToken)
      res.append(sep);
  }
}

WARN_UNUSED_RESULT bool GetAffiliationName(FeatureType const & ft, string & affiliation)
{
  affiliation.clear();

  if (ft.GetName(StringUtf8Multilang::kDefaultCode, affiliation) && !affiliation.empty())
    return true;

  // As a best effort, we try to read an english name if default name is absent.
  if (ft.GetName(StringUtf8Multilang::kEnglishCode, affiliation) && !affiliation.empty())
    return true;
  return false;
}

bool HasSearchIndex(MwmValue const & value) { return value.m_cont.IsExist(SEARCH_INDEX_FILE_TAG); }

bool HasGeometryIndex(MwmValue & value) { return value.m_cont.IsExist(INDEX_FILE_TAG); }

MwmSet::MwmHandle FindWorld(Index const & index, vector<shared_ptr<MwmInfo>> const & infos)
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

strings::UniString AsciiToUniString(char const * s) { return strings::UniString(s, s + strlen(s)); }

bool IsStopWord(strings::UniString const & s)
{
  /// @todo Get all common used stop words and factor out this array into
  /// search_string_utils.cpp module for example.
  static char const * arr[] = {"a", "de", "da", "la"};

  static set<strings::UniString> const kStopWords(
      make_transform_iterator(arr, &AsciiToUniString),
      make_transform_iterator(arr + ARRAY_SIZE(arr), &AsciiToUniString));

  return kStopWords.count(s) > 0;
}

double Area(m2::RectD const & rect) { return rect.IsValid() ? rect.SizeX() * rect.SizeY() : 0; }

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

// Returns shortest distance from the |pivot| to the |rect|.
//
// *NOTE* calculations below are incorrect, because shortest distance
// on the Mercator's plane is not the same as shortest distance on the
// Earth. But we assume that it is not an issue here.
double GetDistanceMeters(m2::PointD const & pivot, m2::RectD const & rect)
{
  if (rect.IsPointInside(pivot))
    return 0.0;

  double distance = numeric_limits<double>::max();
  m2::ProjectionToSection<m2::PointD> proj;

  proj.SetBounds(rect.LeftTop(), rect.RightTop());
  distance = min(distance, MercatorBounds::DistanceOnEarth(pivot, proj(pivot)));

  proj.SetBounds(rect.LeftBottom(), rect.RightBottom());
  distance = min(distance, MercatorBounds::DistanceOnEarth(pivot, proj(pivot)));

  proj.SetBounds(rect.LeftTop(), rect.LeftBottom());
  distance = min(distance, MercatorBounds::DistanceOnEarth(pivot, proj(pivot)));

  proj.SetBounds(rect.RightTop(), rect.RightBottom());
  distance = min(distance, MercatorBounds::DistanceOnEarth(pivot, proj(pivot)));

  return distance;
}

struct KeyedMwmInfo
{
  KeyedMwmInfo(shared_ptr<MwmInfo> const & info, m2::RectD const & pivot) : m_info(info)
  {
    auto const & rect = m_info->m_limitRect;
    m_similarity = GetSimilarity(pivot, rect);
    m_distance = GetDistanceMeters(pivot.Center(), rect);
  }

  bool operator<(KeyedMwmInfo const & rhs) const
  {
    if (m_distance == 0.0 && rhs.m_distance == 0.0)
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
void UniteCBVs(vector<CBV> & cbvs)
{
  while (cbvs.size() > 1)
  {
    size_t i = 0;
    size_t j = 0;
    for (; j + 1 < cbvs.size(); j += 2)
      cbvs[i++] = cbvs[j].Union(cbvs[j + 1]);
    for (; j < cbvs.size(); ++j)
      cbvs[i++] = move(cbvs[j]);
    cbvs.resize(i);
  }
}
}  // namespace

// Geocoder::Params --------------------------------------------------------------------------------
Geocoder::Params::Params() : m_mode(Mode::Everywhere) {}

// Geocoder::Geocoder ------------------------------------------------------------------------------
Geocoder::Geocoder(Index const & index, storage::CountryInfoGetter const & infoGetter,
                   PreRanker & preRanker, my::Cancellable const & cancellable)
  : m_index(index)
  , m_infoGetter(infoGetter)
  , m_cancellable(cancellable)
  , m_model(SearchModel::Instance())
  , m_pivotRectsCache(kPivotRectsCacheSize, m_cancellable, Processor::kMaxViewportRadiusM)
  , m_localityRectsCache(kLocalityRectsCacheSize, m_cancellable)
  , m_filter(nullptr)
  , m_matcher(nullptr)
  , m_finder(m_cancellable)
  , m_lastMatchedRegion(nullptr)
  , m_preRanker(preRanker)
{
  ftypes::IsStreetChecker::Instance().ForEachType([this](uint32_t type) { m_streets.Add(type); });
  ftypes::IsVillageChecker::Instance().ForEachType([this](uint32_t type) { m_villages.Add(type); });
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

  // Remove all category synonyms for streets, as they're extracted
  // individually via LoadStreets.
  for (size_t i = 0; i < m_params.GetNumTokens(); ++i)
  {
    auto & synonyms = m_params.GetTokens(i);
    ASSERT(!synonyms.empty(), ());

    if (IsStreetSynonym(synonyms.front()))
    {
      auto b = synonyms.begin();
      auto e = synonyms.end();
      synonyms.erase(remove_if(b + 1, e,
                               [this](strings::UniString const & synonym) {
                                 return m_streets.HasKey(synonym);
                               }),
                     e);
    }
  }

  LOG(LDEBUG, ("Tokens =", m_params.m_tokens));
  LOG(LDEBUG, ("Prefix tokens =", m_params.m_prefixTokens));
  LOG(LDEBUG, ("Languages =", m_params.m_langs));
}

void Geocoder::GoEverywhere()
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

  if (m_params.GetNumTokens() == 0)
    return;

  vector<shared_ptr<MwmInfo>> infos;
  m_index.GetMwmsInfo(infos);

  GoImpl(infos, false /* inViewport */);
}

void Geocoder::GoInViewport()
{
  if (m_params.GetNumTokens() == 0)
    return;

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
          BaseContext ctx;
          InitBaseContext(ctx);
          FillLocalitiesTable(ctx);
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

    // MatchAroundPivot() should always be matched in mwms
    // intersecting with position and viewport.
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
                     });

      auto it = m_matchersCache.find(m_context->GetId());
      if (it == m_matchersCache.end())
      {
        it = m_matchersCache.insert(make_pair(m_context->GetId(), make_unique<FeaturesLayerMatcher>(
                                                                      m_index, m_cancellable)))
                 .first;
      }
      m_matcher = it->second.get();
      m_matcher->SetContext(m_context.get());

      BaseContext ctx;
      InitBaseContext(ctx);

      if (inViewport)
      {
        auto const viewportCBV =
            RetrieveGeometryFeatures(*m_context, m_params.m_pivot, RECT_ID_PIVOT);
        for (auto & features : ctx.m_features)
          features = features.Intersect(viewportCBV);
      }

      ctx.m_villages = LoadVillages(*m_context);

      auto citiesFromWorld = m_cities;
      FillVillageLocalities(ctx);
      MY_SCOPE_GUARD(remove_villages, [&]()
                     {
                       m_cities = citiesFromWorld;
                     });


      m_lastMatchedRegion = nullptr;
      MatchRegions(ctx, REGION_TYPE_COUNTRY);

      if (index < numIntersectingMaps || m_preRanker.NumSentResults() == 0)
        MatchAroundPivot(ctx);

      if (index + 1 >= numIntersectingMaps)
        m_preRanker.UpdateResults(false /* lastUpdate */);
    };

    // Iterates through all alive mwms and performs geocoding.
    ForEachCountry(infos, processCountry);

    m_preRanker.UpdateResults(true /* lastUpdate */);
  }
  catch (CancelException & e)
  {
  }
}

void Geocoder::ClearCaches()
{
  m_pivotRectsCache.Clear();
  m_localityRectsCache.Clear();

  m_matchersCache.clear();
  m_streetsCache.clear();
  m_postcodes.Clear();
}

void Geocoder::PrepareRetrievalParams(size_t curToken, size_t endToken)
{
  ASSERT_LESS(curToken, endToken, ());
  ASSERT_LESS_OR_EQUAL(endToken, m_params.GetNumTokens(), ());

  m_retrievalParams.m_tokens.clear();
  m_retrievalParams.m_prefixTokens.clear();
  m_retrievalParams.m_types.clear();

  // TODO (@y): possibly it's not cheap to copy vectors of strings.
  // Profile it, and in case of serious performance loss, refactor
  // QueryParams to support subsets of tokens.
  for (size_t i = curToken; i < endToken; ++i)
  {
    if (i < m_params.m_tokens.size())
      m_retrievalParams.m_tokens.push_back(m_params.m_tokens[i]);
    else
      m_retrievalParams.m_prefixTokens = m_params.m_prefixTokens;

    m_retrievalParams.m_types.push_back(m_params.m_types[i]);
  }
}

void Geocoder::InitBaseContext(BaseContext & ctx)
{
  ctx.m_usedTokens.assign(m_params.GetNumTokens(), false);
  ctx.m_numTokens = m_params.GetNumTokens();
  ctx.m_features.resize(ctx.m_numTokens);
  for (size_t i = 0; i < ctx.m_features.size(); ++i)
  {
    PrepareRetrievalParams(i, i + 1);
    ctx.m_features[i] = RetrieveAddressFeatures(*m_context, m_cancellable, m_retrievalParams);
  }
}

void Geocoder::InitLayer(SearchModel::SearchType type, size_t startToken, size_t endToken,
                         FeaturesLayer & layer)
{
  layer.Clear();
  layer.m_type = type;
  layer.m_startToken = startToken;
  layer.m_endToken = endToken;

  JoinQueryTokens(m_params, layer.m_startToken, layer.m_endToken, kUniSpace /* sep */,
                  layer.m_subQuery);
  layer.m_lastTokenIsPrefix = (layer.m_endToken > m_params.m_tokens.size());
}

void Geocoder::FillLocalityCandidates(BaseContext const & ctx, CBV const & filter,
                                      size_t const maxNumLocalities,
                                      vector<Locality> & preLocalities)
{
  preLocalities.clear();

  for (size_t startToken = 0; startToken < ctx.m_numTokens; ++startToken)
  {
    CBV intersection = filter.Intersect(ctx.m_features[startToken]);
    if (intersection.IsEmpty())
      continue;

    CBV unfilteredIntersection = ctx.m_features[startToken];

    for (size_t endToken = startToken + 1; endToken <= ctx.m_numTokens; ++endToken)
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
                               l.m_prob = static_cast<double>(intersection.PopCount()) /
                                          static_cast<double>(unfilteredIntersection.PopCount());
                               preLocalities.push_back(l);
                             });
      }

      if (endToken < ctx.m_numTokens)
      {
        intersection = intersection.Intersect(ctx.m_features[endToken]);
        if (intersection.IsEmpty())
          break;

        unfilteredIntersection = unfilteredIntersection.Intersect(ctx.m_features[endToken]);
      }
    }
  }

  LocalityScorerDelegate delegate(*m_context, m_params);
  LocalityScorer scorer(m_params, delegate);
  scorer.GetTopLocalities(maxNumLocalities, preLocalities);
}

void Geocoder::FillLocalitiesTable(BaseContext const & ctx)
{
  vector<Locality> preLocalities;

  CBV filter;
  filter.SetFull();
  FillLocalityCandidates(ctx, filter, kMaxNumLocalities, preLocalities);

  size_t numCities = 0;
  size_t numStates = 0;
  size_t numCountries = 0;
  for (auto & l : preLocalities)
  {
    FeatureType ft;
    if (!m_context->GetFeature(l.m_featureId, ft))
      continue;

    auto addRegionMaps = [&](size_t & count, size_t maxCount, RegionType type)
    {
      if (count < maxCount && ft.GetFeatureType() == feature::GEOM_POINT)
      {
        string affiliation;
        if (!GetAffiliationName(ft, affiliation))
          return;

        Region region(l, type);
        region.m_center = ft.GetCenter();

        ft.GetName(StringUtf8Multilang::kDefaultCode, region.m_defaultName);
        LOG(LDEBUG, ("Region =", region.m_defaultName));

        m_infoGetter.GetMatchedRegions(affiliation, region.m_ids);
        if (region.m_ids.empty())
        {
          LOG(LWARNING,
              ("Maps not found for region:", region.m_defaultName, "affiliation:", affiliation));
        }

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
        auto const population = ftypes::GetPopulation(ft);
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
    default: break;
    }
  }
}

void Geocoder::FillVillageLocalities(BaseContext const & ctx)
{
  vector<Locality> preLocalities;
  FillLocalityCandidates(ctx, ctx.m_villages /* filter */, kMaxNumVillages, preLocalities);

  size_t numVillages = 0;

  for (auto & l : preLocalities)
  {
    FeatureType ft;
    if (!m_context->GetFeature(l.m_featureId, ft))
      continue;

    if (m_model.GetSearchType(ft) != SearchModel::SEARCH_TYPE_VILLAGE)
      continue;

    // We accept lines and areas as village features.
    auto const center = feature::GetCenter(ft);
    ++numVillages;
    City village(l, SearchModel::SEARCH_TYPE_VILLAGE);

    auto const population = ftypes::GetPopulation(ft);
    auto const radius = ftypes::GetRadiusByPopulation(population);
    village.m_rect = MercatorBounds::RectByCenterXYAndSizeInMeters(center, radius);

#if defined(DEBUG)
    ft.GetName(StringUtf8Multilang::kDefaultCode, village.m_defaultName);
    LOG(LDEBUG, ("Village =", village.m_defaultName, radius, "prob =", village.m_prob));
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
    if (info->GetType() == MwmInfo::COUNTRY && m_params.m_mode == Mode::Downloader)
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

void Geocoder::MatchRegions(BaseContext & ctx, RegionType type)
{
  switch (type)
  {
  case REGION_TYPE_STATE:
    // Tries to skip state matching and go to cities matching.
    // Then, performs states matching.
    MatchCities(ctx);
    break;
  case REGION_TYPE_COUNTRY:
    // Tries to skip country matching and go to states matching.
    // Then, performs countries matching.
    MatchRegions(ctx, REGION_TYPE_STATE);
    break;
  case REGION_TYPE_COUNT: ASSERT(false, ("Invalid region type.")); return;
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
    if (ctx.HasUsedTokensInRange(startToken, endToken))
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

      ScopedMarkTokens mark(ctx.m_usedTokens, startToken, endToken);
      if (ctx.AllTokensUsed())
      {
        // Region matches to search query, we need to emit it as is.
        EmitResult(region, startToken, endToken);
        continue;
      }

      m_lastMatchedRegion = &region;
      MY_SCOPE_GUARD(cleanup, [this]()
                     {
                       m_lastMatchedRegion = nullptr;
                     });
      switch (type)
      {
      case REGION_TYPE_STATE: MatchCities(ctx); break;
      case REGION_TYPE_COUNTRY: MatchRegions(ctx, REGION_TYPE_STATE); break;
      case REGION_TYPE_COUNT: ASSERT(false, ("Invalid region type.")); break;
      }
    }
  }
}

void Geocoder::MatchCities(BaseContext & ctx)
{
  // Localities are ordered my (m_startToken, m_endToken) pairs.
  for (auto const & p : m_cities)
  {
    size_t const startToken = p.first.first;
    size_t const endToken = p.first.second;
    if (ctx.HasUsedTokensInRange(startToken, endToken))
      continue;

    for (auto const & city : p.second)
    {
      BailIfCancelled();

      if (m_lastMatchedRegion &&
          !m_infoGetter.IsBelongToRegions(city.m_rect.Center(), m_lastMatchedRegion->m_ids))
      {
        continue;
      }

      ScopedMarkTokens mark(ctx.m_usedTokens, startToken, endToken);
      if (ctx.AllTokensUsed())
      {
        // City matches to search query, we need to emit it as is.
        EmitResult(city, startToken, endToken);
        continue;
      }

      // No need to search features in the World map.
      if (m_context->GetInfo()->GetType() == MwmInfo::WORLD)
        continue;

      auto cityFeatures = RetrieveGeometryFeatures(*m_context, city.m_rect, RECT_ID_LOCALITY);

      if (cityFeatures.IsEmpty())
        continue;

      LocalityFilter filter(cityFeatures);
      LimitedSearch(ctx, filter);
    }
  }
}

void Geocoder::MatchAroundPivot(BaseContext & ctx)
{
  auto const features = RetrieveGeometryFeatures(*m_context, m_params.m_pivot, RECT_ID_PIVOT);
  ViewportFilter filter(features, m_preRanker.Limit() /* threshold */);
  LimitedSearch(ctx, filter);
}

void Geocoder::LimitedSearch(BaseContext & ctx, FeaturesFilter const & filter)
{
  m_filter = &filter;
  MY_SCOPE_GUARD(resetFilter, [&]()
                 {
                   m_filter = nullptr;
                 });

  if (!ctx.m_streets)
    ctx.m_streets = LoadStreets(*m_context);

  MatchUnclassified(ctx, 0 /* curToken */);

  auto const search = [this, &ctx]()
  {
    GreedilyMatchStreets(ctx);
    MatchPOIsAndBuildings(ctx, 0 /* curToken */);
  };

  WithPostcodes(ctx, search);
  search();
}

template <typename TFn>
void Geocoder::WithPostcodes(BaseContext & ctx, TFn && fn)
{
  size_t const maxPostcodeTokens = GetMaxNumTokensInPostcode();

  for (size_t startToken = 0; startToken != ctx.m_numTokens; ++startToken)
  {
    size_t endToken = startToken;
    for (size_t n = 1; startToken + n <= ctx.m_numTokens && n <= maxPostcodeTokens; ++n)
    {
      if (ctx.m_usedTokens[startToken + n - 1])
        break;

      TokenSlice slice(m_params, startToken, startToken + n);
      auto const isPrefix = startToken + n == ctx.m_numTokens;
      if (LooksLikePostcode(QuerySlice(slice), isPrefix))
        endToken = startToken + n;
    }
    if (startToken == endToken)
      continue;

    auto postcodes =
        RetrievePostcodeFeatures(*m_context, TokenSlice(m_params, startToken, endToken));
    MY_SCOPE_GUARD(cleanup, [&]()
                   {
                     m_postcodes.Clear();
                   });

    if (!postcodes.IsEmpty())
    {
      ScopedMarkTokens mark(ctx.m_usedTokens, startToken, endToken);

      m_postcodes.Clear();
      m_postcodes.m_startToken = startToken;
      m_postcodes.m_endToken = endToken;
      m_postcodes.m_features = move(postcodes);

      fn();
    }
  }
}

void Geocoder::GreedilyMatchStreets(BaseContext & ctx)
{
  vector<StreetsMatcher::Prediction> predictions;
  StreetsMatcher::Go(ctx, *m_filter, m_params, predictions);

  for (auto const & prediction : predictions)
    CreateStreetsLayerAndMatchLowerLayers(ctx, prediction);
}

void Geocoder::CreateStreetsLayerAndMatchLowerLayers(BaseContext & ctx,
                                                     StreetsMatcher::Prediction const & prediction)
{
  ASSERT(m_layers.empty(), ());

  m_layers.emplace_back();
  MY_SCOPE_GUARD(cleanupGuard, bind(&vector<FeaturesLayer>::pop_back, &m_layers));

  auto & layer = m_layers.back();
  InitLayer(SearchModel::SEARCH_TYPE_STREET, prediction.m_startToken, prediction.m_endToken, layer);

  vector<uint32_t> sortedFeatures;
  sortedFeatures.reserve(prediction.m_features.PopCount());
  prediction.m_features.ForEach(MakeBackInsertFunctor(sortedFeatures));
  layer.m_sortedFeatures = &sortedFeatures;

  ScopedMarkTokens mark(ctx.m_usedTokens, prediction.m_startToken, prediction.m_endToken);
  MatchPOIsAndBuildings(ctx, 0 /* curToken */);
}

void Geocoder::MatchPOIsAndBuildings(BaseContext & ctx, size_t curToken)
{
  BailIfCancelled();

  curToken = ctx.SkipUsedTokens(curToken);
  if (curToken == ctx.m_numTokens)
  {
    // All tokens were consumed, find paths through layers, emit
    // features.
    if (m_postcodes.m_features.IsEmpty())
      return FindPaths();

    // When there are no layers but user entered a postcode, we have
    // to emit all features matching to the postcode.
    if (m_layers.size() == 0)
    {
      CBV filtered = m_postcodes.m_features;
      if (m_filter->NeedToFilter(m_postcodes.m_features))
        filtered = m_filter->Filter(m_postcodes.m_features);
      filtered.ForEach([&](uint32_t id)
                       {
                         SearchModel::SearchType searchType;
                         if (GetSearchTypeInGeocoding(ctx, id, searchType))
                         {
                           EmitResult(m_context->GetId(), id, searchType, m_postcodes.m_startToken,
                                      m_postcodes.m_endToken);
                         }
                       });
      return;
    }

    if (!(m_layers.size() == 1 && m_layers[0].m_type == SearchModel::SEARCH_TYPE_STREET))
      return FindPaths();

    // If there're only one street layer but user also entered a
    // postcode, we need to emit all features matching to postcode on
    // the given street, including the street itself.

    // Following code emits streets matched by postcodes, because
    // GreedilyMatchStreets() doesn't (and shouldn't) perform
    // postcodes matching.
    {
      for (auto const & id : *m_layers.back().m_sortedFeatures)
      {
        if (!m_postcodes.m_features.HasBit(id))
          continue;
        EmitResult(m_context->GetId(), id, SearchModel::SEARCH_TYPE_STREET,
                   m_layers.back().m_startToken, m_layers.back().m_endToken);
      }
    }

    // Following code creates a fake layer with buildings and
    // intersects it with the streets layer.
    m_layers.emplace_back();
    MY_SCOPE_GUARD(cleanupGuard, bind(&vector<FeaturesLayer>::pop_back, &m_layers));

    auto & layer = m_layers.back();
    InitLayer(SearchModel::SEARCH_TYPE_BUILDING, m_postcodes.m_startToken, m_postcodes.m_endToken,
              layer);

    vector<uint32_t> features;
    m_postcodes.m_features.ForEach(MakeBackInsertFunctor(features));
    layer.m_sortedFeatures = &features;
    return FindPaths();
  }

  m_layers.emplace_back();
  MY_SCOPE_GUARD(cleanupGuard, bind(&vector<FeaturesLayer>::pop_back, &m_layers));

  // Clusters of features by search type. Each cluster is a sorted
  // list of ids.
  size_t const kNumClusters = SearchModel::SEARCH_TYPE_BUILDING + 1;
  vector<uint32_t> clusters[kNumClusters];

  // Appends |featureId| to the end of the corresponding cluster, if
  // any.
  auto clusterize = [&](uint32_t featureId)
  {
    SearchModel::SearchType searchType;
    if (!GetSearchTypeInGeocoding(ctx, featureId, searchType))
      return;

    // All SEARCH_TYPE_CITY features were filtered in
    // MatchCities().  All SEARCH_TYPE_STREET features were
    // filtered in GreedilyMatchStreets().
    if (searchType < kNumClusters)
    {
      if (m_postcodes.m_features.IsEmpty() || m_postcodes.m_features.HasBit(featureId))
        clusters[searchType].push_back(featureId);
    }
  };

  CBV features;
  features.SetFull();

  // Try to consume [curToken, m_numTokens) tokens range.
  for (size_t n = 1; curToken + n <= ctx.m_numTokens && !ctx.m_usedTokens[curToken + n - 1]; ++n)
  {
    // At this point |features| is the intersection of
    // m_addressFeatures[curToken], m_addressFeatures[curToken + 1],
    // ..., m_addressFeatures[curToken + n - 2].

    BailIfCancelled();

    {
      auto & layer = m_layers.back();
      InitLayer(layer.m_type, curToken, curToken + n, layer);
    }

    features = features.Intersect(ctx.m_features[curToken + n - 1]);

    CBV filtered = features;
    if (m_filter->NeedToFilter(features))
      filtered = m_filter->Filter(features);

    bool const looksLikeHouseNumber = house_numbers::LooksLikeHouseNumber(
        m_layers.back().m_subQuery, m_layers.back().m_lastTokenIsPrefix);

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
        return !filtered.HasBit(featureId);
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
        MatchPOIsAndBuildings(ctx, curToken + n);
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
  sort(sortedLayers.begin(), sortedLayers.end(), my::LessBy(&FeaturesLayer::m_type));

  auto const & innermostLayer = *sortedLayers.front();

  if (!m_postcodes.m_features.IsEmpty())
    m_matcher->SetPostcodes(&m_postcodes.m_features);
  else
    m_matcher->SetPostcodes(nullptr);
  m_finder.ForEachReachableVertex(
      *m_matcher, sortedLayers, [this, &innermostLayer](IntersectionResult const & result)
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

  // Distance and rank will be filled at the end, for all results at once.
  //
  // TODO (@y, @m): need to skip zero rank features that are too
  // distant from the pivot when there're enough results close to the
  // pivot.
  PreRankingInfo info;
  info.m_searchType = type;
  info.m_startToken = startToken;
  info.m_endToken = endToken;

  m_preRanker.Emplace(id, info);
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

void Geocoder::MatchUnclassified(BaseContext & ctx, size_t curToken)
{
  ASSERT(m_layers.empty(), ());

  // We need to match all unused tokens to UNCLASSIFIED features,
  // therefore unused tokens must be adjacent to each other.  For
  // example, as parks are UNCLASSIFIED now, it's ok to match "London
  // Hyde Park", because London will be matched as a city and rest
  // adjacent tokens will be matched to "Hyde Park", whereas it's not
  // ok to match something to "Park London Hyde", because tokens
  // "Park" and "Hyde" are not adjacent.
  if (ctx.NumUnusedTokenGroups() != 1)
    return;

  CBV allFeatures;
  allFeatures.SetFull();

  auto startToken = curToken;
  for (curToken = ctx.SkipUsedTokens(curToken);
       curToken < ctx.m_numTokens && !ctx.m_usedTokens[curToken]; ++curToken)
  {
    allFeatures = allFeatures.Intersect(ctx.m_features[curToken]);
  }

  if (m_filter->NeedToFilter(allFeatures))
    allFeatures = m_filter->Filter(allFeatures);

  auto emitUnclassified = [&](uint32_t featureId)
  {
    SearchModel::SearchType searchType;
    if (!GetSearchTypeInGeocoding(ctx, featureId, searchType))
      return;
    if (searchType == SearchModel::SEARCH_TYPE_UNCLASSIFIED)
      EmitResult(m_context->GetId(), featureId, searchType, startToken, curToken);
  };
  allFeatures.ForEach(emitUnclassified);
}

CBV Geocoder::LoadCategories(MwmContext & context, CategoriesSet const & categories)
{
  ASSERT(context.m_handle.IsAlive(), ());
  ASSERT(HasSearchIndex(context.m_value), ());

  m_retrievalParams.m_tokens.resize(1);
  m_retrievalParams.m_tokens[0].resize(1);

  m_retrievalParams.m_prefixTokens.clear();

  m_retrievalParams.m_types.resize(1);
  m_retrievalParams.m_types[0].resize(1);

  vector<CBV> cbvs;

  categories.ForEach([&](strings::UniString const & key, uint32_t const type) {
    m_retrievalParams.m_tokens[0][0] = key;
    m_retrievalParams.m_types[0][0] = type;

    CBV cbv(RetrieveAddressFeatures(context, m_cancellable, m_retrievalParams));
    if (!cbv.IsEmpty())
      cbvs.push_back(move(cbv));
  });

  UniteCBVs(cbvs);
  if (cbvs.empty())
    cbvs.emplace_back();

  return move(cbvs[0]);
}

CBV Geocoder::LoadStreets(MwmContext & context)
{
  if (!context.m_handle.IsAlive() || !HasSearchIndex(context.m_value))
    return CBV();

  auto mwmId = context.m_handle.GetId();
  auto const it = m_streetsCache.find(mwmId);
  if (it != m_streetsCache.cend())
    return it->second;

  auto streets = LoadCategories(context, m_streets);
  m_streetsCache[mwmId] = streets;
  return streets;
}

CBV Geocoder::LoadVillages(MwmContext & context)
{
  if (!context.m_handle.IsAlive() || !HasSearchIndex(context.m_value))
    return CBV();
  return LoadCategories(context, m_villages);
}

CBV Geocoder::RetrievePostcodeFeatures(MwmContext const & context, TokenSlice const & slice)
{
  return CBV(::search::RetrievePostcodeFeatures(context, m_cancellable, slice));
}

CBV Geocoder::RetrieveGeometryFeatures(MwmContext const & context, m2::RectD const & rect,
                                       RectId id)
{
  switch (id)
  {
  case RECT_ID_PIVOT: return m_pivotRectsCache.Get(context, rect, m_params.m_scale);
  case RECT_ID_LOCALITY: return m_localityRectsCache.Get(context, rect, m_params.m_scale);
  case RECT_ID_COUNT: ASSERT(false, ("Invalid RectId.")); return CBV();
  }
}

bool Geocoder::GetSearchTypeInGeocoding(BaseContext const & ctx, uint32_t featureId,
                                        SearchModel::SearchType & searchType)
{
  if (ctx.m_streets.HasBit(featureId))
  {
    searchType = SearchModel::SEARCH_TYPE_STREET;
    return true;
  }
  if (ctx.m_villages.HasBit(featureId))
  {
    searchType = SearchModel::SEARCH_TYPE_VILLAGE;
    return true;
  }

  FeatureType feature;
  if (m_context->GetFeature(featureId, feature))
  {
    searchType = m_model.GetSearchType(feature);
    return true;
  }

  return false;
}

string DebugPrint(Geocoder::Locality const & locality)
{
  ostringstream os;
  os << "Locality [" << DebugPrint(locality.m_countryId) << ", featureId=" << locality.m_featureId
     << ", startToken=" << locality.m_startToken << ", endToken=" << locality.m_endToken << "]";
  return os.str();
}
}  // namespace search
