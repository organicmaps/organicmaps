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
#include "search/tracer.hpp"
#include "search/utils.hpp"

#include "indexer/classificator.hpp"
#include "indexer/data_source.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/postcodes_matcher.hpp"
#include "indexer/rank_table.hpp"
#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "storage/country_info_getter.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "platform/preferred_languages.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/pprof.hpp"
#include "base/random.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"
#include "std/iterator.hpp"
#include "std/random.hpp"
#include "std/sstream.hpp"
#include "std/transform_iterator.hpp"
#include "std/unique_ptr.hpp"

#include "defines.hpp"

#if defined(DEBUG)
#include "base/timer.hpp"
#endif

#include <memory>

using namespace strings;

namespace search
{
namespace
{
size_t constexpr kMaxNumCities = 5;
size_t constexpr kMaxNumStates = 5;
size_t constexpr kMaxNumVillages = 5;
size_t constexpr kMaxNumCountries = 5;
double constexpr kMaxViewportRadiusM = 50.0 * 1000;

// This constant limits number of localities that will be extracted
// from World map.  Villages are not counted here as they're not
// included into World map.
// @vng Set this value to possible maximum.
size_t const kMaxNumLocalities = LocalityScorer::kDefaultReadLimit;

size_t constexpr kPivotRectsCacheSize = 10;
size_t constexpr kLocalityRectsCacheSize = 10;

UniString const kUniSpace(MakeUniString(" "));

struct ScopedMarkTokens
{
  static BaseContext::TokenType constexpr kUnused = BaseContext::TOKEN_TYPE_COUNT;

  ScopedMarkTokens(vector<BaseContext::TokenType> & tokens, BaseContext::TokenType type,
                   TokenRange const & range)
    : m_tokens(tokens), m_type(type), m_range(range)
  {
    ASSERT(m_range.IsValid(), ());
    ASSERT_LESS_OR_EQUAL(m_range.End(), m_tokens.size(), ());
#if defined(DEBUG)
    for (size_t i : m_range)
      ASSERT_EQUAL(m_tokens[i], kUnused, (i));
#endif
    fill(m_tokens.begin() + m_range.Begin(), m_tokens.begin() + m_range.End(), m_type);
  }

  ~ScopedMarkTokens()
  {
#if defined(DEBUG)
    for (size_t i : m_range)
      ASSERT_EQUAL(m_tokens[i], m_type, (i));
#endif
    fill(m_tokens.begin() + m_range.Begin(), m_tokens.begin() + m_range.End(), kUnused);
  }

  vector<search::BaseContext::TokenType> & m_tokens;
  search::BaseContext::TokenType const m_type;
  TokenRange const m_range;
};

// static
BaseContext::TokenType constexpr ScopedMarkTokens::kUnused;

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
    m_table = search::RankTable::Load(m_value.m_cont, SEARCH_RANKS_FILE_TAG);
    if (!m_table)
      m_table = make_unique<search::DummyRankTable>();
  }

  MwmValue const & m_value;
  mutable unique_ptr<search::RankTable> m_table;
};

class LocalityScorerDelegate : public LocalityScorer::Delegate
{
public:
  LocalityScorerDelegate(MwmContext const & context, Geocoder::Params const & params,
                         base::Cancellable const & cancellable)
    : m_context(context)
    , m_params(params)
    , m_cancellable(cancellable)
    , m_retrieval(m_context, m_cancellable)
    , m_ranks(m_context.m_value)
  {
  }

  // LocalityScorer::Delegate overrides:
  void GetNames(uint32_t featureId, vector<string> & names) const override
  {
    FeatureType ft;
    if (!m_context.GetFeature(featureId, ft))
      return;
    for (auto const lang : m_params.GetLangs())
    {
      string name;
      if (ft.GetName(lang, name))
        names.push_back(name);
    }
  }

  uint8_t GetRank(uint32_t featureId) const override { return m_ranks.Get(featureId); }

  CBV GetMatchedFeatures(strings::UniString const & token, bool isPrefix) const override
  {
    if (isPrefix)
    {
      SearchTrieRequest<strings::PrefixDFAModifier<strings::UniStringDFA>> request;
      request.m_names.emplace_back(strings::UniStringDFA(token));
      request.SetLangs(m_params.GetLangs());
      return CBV{m_retrieval.RetrieveAddressFeatures(request)};
    }
    else
    {
      SearchTrieRequest<strings::UniStringDFA> request;
      request.m_names.emplace_back(token);
      request.SetLangs(m_params.GetLangs());
      return CBV{m_retrieval.RetrieveAddressFeatures(request)};
    }
  }

private:
  MwmContext const & m_context;
  Geocoder::Params const & m_params;
  base::Cancellable const & m_cancellable;

  Retrieval m_retrieval;

  LazyRankTable m_ranks;
};

void JoinQueryTokens(QueryParams const & params, TokenRange const & range, UniString const & sep,
                     UniString & res)
{
  ASSERT(range.IsValid(), (range));
  for (size_t i : range)
  {
    res.append(params.GetToken(i).m_original);
    if (i + 1 != range.End())
      res.append(sep);
  }
}

WARN_UNUSED_RESULT bool GetAffiliationName(FeatureType & ft, string & affiliation)
{
  affiliation.clear();

  if (ft.GetName(StringUtf8Multilang::kDefaultCode, affiliation) && !affiliation.empty())
    return true;

  // As a best effort, we try to read an english name if default name is absent.
  if (ft.GetName(StringUtf8Multilang::kEnglishCode, affiliation) && !affiliation.empty())
    return true;
  return false;
}

double Area(m2::RectD const & rect) { return rect.IsValid() ? rect.SizeX() * rect.SizeY() : 0; }

// Computes the average similarity between |rect| and |pivot|. By
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

  rect.ForEachSide([&](m2::PointD const & a, m2::PointD const & b) {
    m2::ParametrizedSegment<m2::PointD> segment(a, b);
    distance = min(distance, MercatorBounds::DistanceOnEarth(pivot, segment.ClosestPointTo(pivot)));
  });

  return distance;
}

struct KeyedMwmInfo
{
  KeyedMwmInfo(shared_ptr<MwmInfo> const & info, boost::optional<m2::PointD> const & position,
               m2::RectD const & pivot, bool inViewport)
    : m_info(info)
  {
    auto const & rect = m_info->m_bordersRect;
    m_similarity = GetSimilarity(pivot, rect);
    m_distance = GetDistanceMeters(pivot.Center(), rect);
    if (!inViewport && position && rect.IsPointInside(*position))
      m_distance = 0.0;
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
// In viewport search mode, prefers mwms that contain the user's position.
size_t OrderCountries(boost::optional<m2::PointD> const & position, m2::RectD const & pivot,
                      bool inViewport, vector<shared_ptr<MwmInfo>> & infos)
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
    keyedInfos.emplace_back(info, position, pivot, inViewport);
  sort(keyedInfos.begin(), keyedInfos.end());

  infos.clear();
  for (auto const & info : keyedInfos)
    infos.emplace_back(info.m_info);

  auto intersects = [&](shared_ptr<MwmInfo> const & info) -> bool {
    if (!inViewport && position && info->m_bordersRect.IsPointInside(*position))
      return true;
    return pivot.IsIntersect(info->m_bordersRect);
  };
  auto const sep = stable_partition(infos.begin(), infos.end(), intersects);
  return distance(infos.begin(), sep);
}
}  // namespace

// Geocoder::Geocoder ------------------------------------------------------------------------------
Geocoder::Geocoder(DataSource const & dataSource, storage::CountryInfoGetter const & infoGetter,
                   CategoriesHolder const & categories,
                   CitiesBoundariesTable const & citiesBoundaries, PreRanker & preRanker,
                   VillagesCache & villagesCache, base::Cancellable const & cancellable)
  : m_dataSource(dataSource)
  , m_infoGetter(infoGetter)
  , m_categories(categories)
  , m_streetsCache(cancellable)
  , m_villagesCache(villagesCache)
  , m_hotelsCache(cancellable)
  , m_foodCache(cancellable)
  , m_hotelsFilter(m_hotelsCache)
  , m_cuisineFilter(m_foodCache)
  , m_cancellable(cancellable)
  , m_citiesBoundaries(citiesBoundaries)
  , m_pivotRectsCache(kPivotRectsCacheSize, m_cancellable, kMaxViewportRadiusM)
  , m_localityRectsCache(kLocalityRectsCacheSize, m_cancellable)
  , m_filter(nullptr)
  , m_matcher(nullptr)
  , m_finder(m_cancellable)
  , m_preRanker(preRanker)
{
}

Geocoder::~Geocoder() {}

void Geocoder::SetParams(Params const & params)
{
  if (params.IsCategorialRequest())
  {
    SetParamsForCategorialSearch(params);
    return;
  }

  m_params = params;

  m_tokenRequests.clear();
  m_prefixTokenRequest.Clear();
  for (size_t i = 0; i < m_params.GetNumTokens(); ++i)
  {
    if (!m_params.IsPrefixToken(i))
    {
      m_tokenRequests.emplace_back();
      auto & request = m_tokenRequests.back();
      m_params.GetToken(i).ForEach([&request](UniString const & s) {
        request.m_names.emplace_back(BuildLevenshteinDFA(s));
      });
      for (auto const & index : m_params.GetTypeIndices(i))
        request.m_categories.emplace_back(FeatureTypeToString(index));
      request.SetLangs(m_params.GetLangs());
    }
    else
    {
      auto & request = m_prefixTokenRequest;
      m_params.GetToken(i).ForEach([&request](UniString const & s) {
        request.m_names.emplace_back(BuildLevenshteinDFA(s));
      });
      for (auto const & index : m_params.GetTypeIndices(i))
        request.m_categories.emplace_back(FeatureTypeToString(index));
      request.SetLangs(m_params.GetLangs());
    }
  }

  LOG(LDEBUG, (static_cast<QueryParams const &>(m_params)));
}

void Geocoder::GoEverywhere()
{
// TODO (@y): remove following code as soon as Geocoder::Go() will
// work fast for most cases (significantly less than 1 second).
#if defined(DEBUG)
  base::Timer timer;
  SCOPE_GUARD(printDuration, [&timer]() {
    LOG(LINFO, ("Total geocoding time:", timer.ElapsedSeconds(), "seconds"));
  });
#endif

  if (m_params.GetNumTokens() == 0)
    return;

  vector<shared_ptr<MwmInfo>> infos;
  m_dataSource.GetMwmsInfo(infos);

  GoImpl(infos, false /* inViewport */);
}

void Geocoder::GoInViewport()
{
  if (m_params.GetNumTokens() == 0)
    return;

  vector<shared_ptr<MwmInfo>> infos;
  m_dataSource.GetMwmsInfo(infos);

  base::EraseIf(infos, [this](shared_ptr<MwmInfo> const & info) {
    return !m_params.m_pivot.IsIntersect(info->m_bordersRect);
  });

  GoImpl(infos, true /* inViewport */);
}

void Geocoder::Finish(bool cancelled)
{
  m_preRanker.Finish(cancelled);
}

void Geocoder::ClearCaches()
{
  m_pivotRectsCache.Clear();
  m_localityRectsCache.Clear();

  m_matchersCache.clear();
  m_streetsCache.Clear();
  m_hotelsCache.Clear();
  m_foodCache.Clear();
  m_hotelsFilter.ClearCaches();
  m_cuisineFilter.ClearCaches();
  m_postcodes.Clear();
}

void Geocoder::SetParamsForCategorialSearch(Params const & params)
{
  m_params = params;

  m_tokenRequests.clear();
  m_prefixTokenRequest.Clear();

  ASSERT(!m_params.LastTokenIsPrefix(), ());

  LOG(LDEBUG, (static_cast<QueryParams const &>(m_params)));
}

void Geocoder::GoImpl(vector<shared_ptr<MwmInfo>> & infos, bool inViewport)
{
  // base::PProf pprof("/tmp/geocoder.prof");

  // Tries to find world and fill localities table.
  {
    m_cities.clear();
    for (auto & regions : m_regions)
      regions.clear();
    MwmSet::MwmHandle handle = FindWorld(m_dataSource, infos);
    if (handle.IsAlive())
    {
      auto & value = *handle.GetValue<MwmValue>();

      // All MwmIds are unique during the application lifetime, so
      // it's ok to save MwmId.
      m_worldId = handle.GetId();
      m_context = make_unique<MwmContext>(move(handle));

      if (value.HasSearchIndex())
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
  size_t const numIntersectingMaps =
      OrderCountries(m_params.m_position, m_params.m_pivot, inViewport, infos);

  // MatchAroundPivot() should always be matched in mwms
  // intersecting with position and viewport.
  auto processCountry = [&](size_t index, unique_ptr<MwmContext> context) {
    ASSERT(context, ());
    m_context = move(context);

    SCOPE_GUARD(cleanup, [&]() {
      LOG(LDEBUG, (m_context->GetName(), "geocoding complete."));
      m_matcher->OnQueryFinished();
      m_matcher = nullptr;
      m_context.reset();
    });

    auto it = m_matchersCache.find(m_context->GetId());
    if (it == m_matchersCache.end())
    {
      it = m_matchersCache
               .insert(make_pair(m_context->GetId(),
                                 std::make_unique<FeaturesLayerMatcher>(m_dataSource, m_cancellable)))
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

    ctx.m_villages = m_villagesCache.Get(*m_context);

    auto citiesFromWorld = m_cities;
    FillVillageLocalities(ctx);
    SCOPE_GUARD(remove_villages, [&]() { m_cities = citiesFromWorld; });

    bool const intersectsPivot = index < numIntersectingMaps;
    if (m_params.IsCategorialRequest())
    {
      MatchCategories(ctx, intersectsPivot);
    }
    else
    {
      MatchRegions(ctx, Region::TYPE_COUNTRY);

      if (intersectsPivot || m_preRanker.NumSentResults() == 0)
        MatchAroundPivot(ctx);
    }

    if (index + 1 >= numIntersectingMaps)
      m_preRanker.UpdateResults(false /* lastUpdate */);
  };

  // Iterates through all alive mwms and performs geocoding.
  ForEachCountry(infos, processCountry);
}

void Geocoder::InitBaseContext(BaseContext & ctx)
{
  Retrieval retrieval(*m_context, m_cancellable);

  ctx.m_tokens.assign(m_params.GetNumTokens(), BaseContext::TOKEN_TYPE_COUNT);
  ctx.m_numTokens = m_params.GetNumTokens();
  ctx.m_features.resize(ctx.m_numTokens);
  for (size_t i = 0; i < ctx.m_features.size(); ++i)
  {
    if (m_params.IsCategorialRequest())
    {
      // Implementation-wise, the simplest way to match a feature by
      // its category bypassing the matching by name is by using a CategoriesCache.
      CategoriesCache cache(m_params.m_preferredTypes, m_cancellable);
      ctx.m_features[i] = cache.Get(*m_context);
    }
    else if (m_params.IsPrefixToken(i))
    {
      ctx.m_features[i] = retrieval.RetrieveAddressFeatures(m_prefixTokenRequest);
    }
    else
    {
      ctx.m_features[i] = retrieval.RetrieveAddressFeatures(m_tokenRequests[i]);
    }
  }

  ctx.m_hotelsFilter = m_hotelsFilter.MakeScopedFilter(*m_context, m_params.m_hotelsFilter);
  ctx.m_cuisineFilter = m_cuisineFilter.MakeScopedFilter(*m_context, m_params.m_cuisineTypes);
}

void Geocoder::InitLayer(Model::Type type, TokenRange const & tokenRange, FeaturesLayer & layer)
{
  layer.Clear();
  layer.m_type = type;
  layer.m_tokenRange = tokenRange;

  JoinQueryTokens(m_params, layer.m_tokenRange, kUniSpace /* sep */, layer.m_subQuery);
  layer.m_lastTokenIsPrefix =
      !layer.m_tokenRange.Empty() && m_params.IsPrefixToken(layer.m_tokenRange.End() - 1);
}

void Geocoder::FillLocalityCandidates(BaseContext const & ctx, CBV const & filter,
                                      size_t const maxNumLocalities,
                                      vector<Locality> & preLocalities)
{
  // todo(@m) "food moscow" should be a valid categorial request.
  if (m_params.IsCategorialRequest())
  {
    preLocalities.clear();
    return;
  }

  LocalityScorerDelegate delegate(*m_context, m_params, m_cancellable);
  LocalityScorer scorer(m_params, delegate);
  scorer.GetTopLocalities(m_context->GetId(), ctx, filter, maxNumLocalities, preLocalities);
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

    auto addRegionMaps = [&](size_t maxCount, Region::Type type, size_t & count)
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
        m_regions[type][l.m_tokenRange].push_back(region);
      }
    };

    switch (m_model.GetType(ft))
    {
    case Model::TYPE_CITY:
    {
      if (numCities < kMaxNumCities && ft.GetFeatureType() == feature::GEOM_POINT)
      {
        ++numCities;

        City city(l, Model::TYPE_CITY);

        CitiesBoundariesTable::Boundaries boundaries;
        bool haveBoundary = false;
        if (m_citiesBoundaries.Get(ft.GetID(), boundaries))
        {
          city.m_rect = boundaries.GetLimitRect();
          if (city.m_rect.IsValid())
            haveBoundary = true;
        }

        if (!haveBoundary)
        {
          auto const center = feature::GetCenter(ft);
          auto const population = ftypes::GetPopulation(ft);
          auto const radius = ftypes::GetRadiusByPopulation(population);
          city.m_rect = MercatorBounds::RectByCenterXYAndSizeInMeters(center, radius);
        }

#if defined(DEBUG)
        ft.GetName(StringUtf8Multilang::kDefaultCode, city.m_defaultName);
        LOG(LINFO,
            ("City =", city.m_defaultName, "rect =", city.m_rect, "rect source:", haveBoundary ? "table" : "population",
             "sizeX =",
             MercatorBounds::DistanceOnEarth(city.m_rect.LeftTop(), city.m_rect.RightTop()),
             "sizeY =",
             MercatorBounds::DistanceOnEarth(city.m_rect.LeftTop(), city.m_rect.LeftBottom())));
#endif

        m_cities[city.m_tokenRange].push_back(city);
      }
      break;
    }
    case Model::TYPE_STATE:
    {
      addRegionMaps(kMaxNumStates, Region::TYPE_STATE, numStates);
      break;
    }
    case Model::TYPE_COUNTRY:
    {
      addRegionMaps(kMaxNumCountries, Region::TYPE_COUNTRY, numCountries);
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

    if (m_model.GetType(ft) != Model::TYPE_VILLAGE)
      continue;

    // We accept lines and areas as village features.
    auto const center = feature::GetCenter(ft);
    ++numVillages;
    City village(l, Model::TYPE_VILLAGE);

    auto const population = ftypes::GetPopulation(ft);
    auto const radius = ftypes::GetRadiusByPopulation(population);
    village.m_rect = MercatorBounds::RectByCenterXYAndSizeInMeters(center, radius);

#if defined(DEBUG)
    ft.GetName(StringUtf8Multilang::kDefaultCode, village.m_defaultName);
    LOG(LDEBUG, ("Village =", village.m_defaultName, "radius =", radius));
#endif

    m_cities[village.m_tokenRange].push_back(village);
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

    auto handle = m_dataSource.GetMwmHandleById(MwmSet::MwmId(info));
    if (!handle.IsAlive())
      continue;
    auto & value = *handle.GetValue<MwmValue>();
    if (!value.HasSearchIndex() || !value.HasGeometryIndex())
      continue;
    fn(i, make_unique<MwmContext>(move(handle)));
  }
}

void Geocoder::MatchCategories(BaseContext & ctx, bool aroundPivot)
{
  auto features = ctx.m_features[0];

  if (aroundPivot)
  {
    auto const pivotFeatures = RetrieveGeometryFeatures(*m_context, m_params.m_pivot, RECT_ID_PIVOT);
    ViewportFilter filter(pivotFeatures, m_preRanker.Limit() /* threshold */);
    features = filter.Filter(features);
  }

  auto emit = [&](uint64_t bit) {
    auto const featureId = base::asserted_cast<uint32_t>(bit);
    Model::Type type;
    if (!GetTypeInGeocoding(ctx, featureId, type))
      return;

    EmitResult(ctx, m_context->GetId(), featureId, type, TokenRange(0, ctx.m_numTokens), nullptr /* geoParts */,
               true /* allTokensUsed */);
  };

  // Features have been retrieved from the search index
  // using the exact (non-fuzzy) matching and intersected
  // with viewport, if needed. Every such feature is relevant.
  features.ForEach(emit);
}

void Geocoder::MatchRegions(BaseContext & ctx, Region::Type type)
{
  switch (type)
  {
  case Region::TYPE_STATE:
    // Tries to skip state matching and go to cities matching.
    // Then, performs states matching.
    MatchCities(ctx);
    break;
  case Region::TYPE_COUNTRY:
    // Tries to skip country matching and go to states matching.
    // Then, performs countries matching.
    MatchRegions(ctx, Region::TYPE_STATE);
    break;
  case Region::TYPE_COUNT: ASSERT(false, ("Invalid region type.")); return;
  }

  auto const & regions = m_regions[type];

  auto const & fileName = m_context->GetName();
  bool const isWorld = m_context->GetInfo()->GetType() == MwmInfo::WORLD;

  // Try to match regions.
  for (auto const & p : regions)
  {
    BailIfCancelled();

    auto const & tokenRange = p.first;
    if (ctx.HasUsedTokensInRange(tokenRange))
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
        matches = ctx.m_regions.empty() ||
                  m_infoGetter.IsBelongToRegions(region.m_center, ctx.m_regions.back()->m_ids);
      }
      else
      {
        matches = m_infoGetter.IsBelongToRegions(fileName, region.m_ids);
      }

      if (!matches)
        continue;

      ctx.m_regions.push_back(&region);
      SCOPE_GUARD(cleanup, [&ctx]() { ctx.m_regions.pop_back(); });

      ScopedMarkTokens mark(ctx.m_tokens, BaseContext::FromRegionType(type), tokenRange);

      if (ctx.AllTokensUsed())
      {
        // Region matches to search query, we need to emit it as is.
        EmitResult(ctx, region, tokenRange, true /* allTokensUsed */);
        continue;
      }

      switch (type)
      {
      case Region::TYPE_STATE: MatchCities(ctx); break;
      case Region::TYPE_COUNTRY: MatchRegions(ctx, Region::TYPE_STATE); break;
      case Region::TYPE_COUNT: ASSERT(false, ("Invalid region type.")); break;
      }
    }
  }
}

void Geocoder::MatchCities(BaseContext & ctx)
{
  ASSERT(!ctx.m_city, ());

  // Localities are ordered my (m_startToken, m_endToken) pairs.
  for (auto const & p : m_cities)
  {
    auto const & tokenRange = p.first;
    if (ctx.HasUsedTokensInRange(tokenRange))
      continue;

    for (auto const & city : p.second)
    {
      BailIfCancelled();

      if (!ctx.m_regions.empty() &&
          !m_infoGetter.IsBelongToRegions(city.m_rect.Center(), ctx.m_regions.back()->m_ids))
      {
        continue;
      }

      ScopedMarkTokens mark(ctx.m_tokens, BaseContext::TOKEN_TYPE_CITY, tokenRange);
      ctx.m_city = &city;
      SCOPE_GUARD(cleanup, [&ctx]() { ctx.m_city = nullptr; });

      if (ctx.AllTokensUsed())
      {
        // City matches to search query, we need to emit it as is.
        EmitResult(ctx, city, tokenRange, true /* allTokensUsed */);
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
  SCOPE_GUARD(resetFilter, [&]() { m_filter = nullptr; });

  if (!ctx.m_streets)
    ctx.m_streets = m_streetsCache.Get(*m_context);

  MatchUnclassified(ctx, 0 /* curToken */);

  auto const search = [this, &ctx]() {
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
      if (ctx.IsTokenUsed(startToken + n - 1))
        break;

      TokenSlice slice(m_params, TokenRange(startToken, startToken + n));
      auto const isPrefix = startToken + n == ctx.m_numTokens;
      if (LooksLikePostcode(QuerySlice(slice), isPrefix))
        endToken = startToken + n;
    }
    if (startToken == endToken)
      continue;

    TokenRange const tokenRange(startToken, endToken);

    auto postcodes = RetrievePostcodeFeatures(*m_context, TokenSlice(m_params, tokenRange));
    SCOPE_GUARD(cleanup, [&]() { m_postcodes.Clear(); });

    if (!postcodes.IsEmpty())
    {
      ScopedMarkTokens mark(ctx.m_tokens, BaseContext::TOKEN_TYPE_POSTCODE, tokenRange);

      m_postcodes.Clear();
      m_postcodes.m_tokenRange = tokenRange;
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
  auto & layers = ctx.m_layers;
  ASSERT(layers.empty(), ());

  layers.emplace_back();
  SCOPE_GUARD(cleanupGuard, bind(&vector<FeaturesLayer>::pop_back, &layers));

  auto & layer = layers.back();
  InitLayer(Model::TYPE_STREET, prediction.m_tokenRange, layer);

  vector<uint32_t> sortedFeatures;
  sortedFeatures.reserve(base::checked_cast<size_t>(prediction.m_features.PopCount()));
  prediction.m_features.ForEach([&sortedFeatures](uint64_t bit) {
    sortedFeatures.push_back(base::asserted_cast<uint32_t>(bit));
  });
  layer.m_sortedFeatures = &sortedFeatures;

  ScopedMarkTokens mark(ctx.m_tokens, BaseContext::TOKEN_TYPE_STREET, prediction.m_tokenRange);
//  size_t const numEmitted = ctx.m_numEmitted;
  MatchPOIsAndBuildings(ctx, 0 /* curToken */);

  // A relaxed best effort parse: at least show the street if we can find one.
//  if (numEmitted == ctx.m_numEmitted)
//    FindPaths(ctx);
}

void Geocoder::MatchPOIsAndBuildings(BaseContext & ctx, size_t curToken)
{
  BailIfCancelled();

  auto & layers = ctx.m_layers;

  curToken = ctx.SkipUsedTokens(curToken);
  if (curToken == ctx.m_numTokens)
  {
    // All tokens were consumed, find paths through layers, emit
    // features.
    if (m_postcodes.m_features.IsEmpty())
      return FindPaths(ctx);

    // When there are no layers but user entered a postcode, we have
    // to emit all features matching to the postcode.
    if (layers.size() == 0)
    {
      CBV filtered = m_postcodes.m_features;
      if (m_filter->NeedToFilter(m_postcodes.m_features))
        filtered = m_filter->Filter(m_postcodes.m_features);
      filtered.ForEach([&](uint64_t bit) {
        auto const featureId = base::asserted_cast<uint32_t>(bit);
        Model::Type type;
        if (GetTypeInGeocoding(ctx, featureId, type))
        {
          EmitResult(ctx, m_context->GetId(), featureId, type, m_postcodes.m_tokenRange,
                     nullptr /* geoParts */, true /* allTokensUsed */);
        }
      });
      return;
    }

    if (!(layers.size() == 1 && layers[0].m_type == Model::TYPE_STREET))
      return FindPaths(ctx);

    // If there're only one street layer but user also entered a
    // postcode, we need to emit all features matching to postcode on
    // the given street, including the street itself.

    // Following code emits streets matched by postcodes, because
    // GreedilyMatchStreets() doesn't (and shouldn't) perform
    // postcodes matching.
    {
      for (auto const & id : *layers.back().m_sortedFeatures)
      {
        if (!m_postcodes.m_features.HasBit(id))
          continue;
        EmitResult(ctx, m_context->GetId(), id, Model::TYPE_STREET, layers.back().m_tokenRange,
                   nullptr /* geoParts */, true /* allTokensUsed */);
      }
    }

    // Following code creates a fake layer with buildings and
    // intersects it with the streets layer.
    layers.emplace_back();
    SCOPE_GUARD(cleanupGuard, bind(&vector<FeaturesLayer>::pop_back, &layers));

    auto & layer = layers.back();
    InitLayer(Model::TYPE_BUILDING, m_postcodes.m_tokenRange, layer);

    vector<uint32_t> features;
    m_postcodes.m_features.ForEach([&features](uint64_t bit) {
      features.push_back(base::asserted_cast<uint32_t>(bit));
    });
    layer.m_sortedFeatures = &features;
    return FindPaths(ctx);
  }

  layers.emplace_back();
  SCOPE_GUARD(cleanupGuard, bind(&vector<FeaturesLayer>::pop_back, &layers));

  // Clusters of features by search type. Each cluster is a sorted
  // list of ids.
  size_t const kNumClusters = Model::TYPE_BUILDING + 1;
  vector<uint32_t> clusters[kNumClusters];

  // Appends |featureId| to the end of the corresponding cluster, if
  // any.
  auto clusterize = [&](uint64_t bit)
  {
    auto const featureId = base::asserted_cast<uint32_t>(bit);
    Model::Type type;
    if (!GetTypeInGeocoding(ctx, featureId, type))
      return;

    // All TYPE_CITY features were filtered in MatchCities().  All
    // TYPE_STREET features were filtered in GreedilyMatchStreets().
    if (type < kNumClusters)
    {
      if (m_postcodes.m_features.IsEmpty() || m_postcodes.m_features.HasBit(featureId))
        clusters[type].push_back(featureId);
    }
  };

  CBV features;
  features.SetFull();

  // Try to consume [curToken, m_numTokens) tokens range.
  for (size_t n = 1; curToken + n <= ctx.m_numTokens && !ctx.IsTokenUsed(curToken + n - 1); ++n)
  {
    // At this point |features| is the intersection of
    // m_addressFeatures[curToken], m_addressFeatures[curToken + 1],
    // ..., m_addressFeatures[curToken + n - 2].

    BailIfCancelled();

    {
      auto & layer = layers.back();
      InitLayer(layer.m_type, TokenRange(curToken, curToken + n), layer);
    }

    features = features.Intersect(ctx.m_features[curToken + n - 1]);

    CBV filtered = features;
    if (m_filter->NeedToFilter(features))
      filtered = m_filter->Filter(features);

    bool const looksLikeHouseNumber = house_numbers::LooksLikeHouseNumber(
        layers.back().m_subQuery, layers.back().m_lastTokenIsPrefix);
    if (filtered.IsEmpty() && !looksLikeHouseNumber)
      break;

    if (n == 1)
    {
      filtered.ForEach(clusterize);
    }
    else
    {
      auto noFeature = [&filtered](uint64_t bit) -> bool
      {
        return !filtered.HasBit(bit);
      };
      for (auto & cluster : clusters)
        base::EraseIf(cluster, noFeature);

      size_t curs[kNumClusters] = {};
      size_t ends[kNumClusters];
      for (size_t i = 0; i < kNumClusters; ++i)
        ends[i] = clusters[i].size();
      filtered.ForEach([&](uint64_t bit)
                       {
                         auto const featureId = base::asserted_cast<uint32_t>(bit);
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
      auto & layer = layers.back();
      layer.m_sortedFeatures = &clusters[i];

      if (i == Model::TYPE_BUILDING)
      {
        if (layer.m_sortedFeatures->empty() && !looksLikeHouseNumber)
          continue;
      }
      else if (layer.m_sortedFeatures->empty())
      {
        continue;
      }

      layer.m_type = static_cast<Model::Type>(i);
      ScopedMarkTokens mark(ctx.m_tokens, BaseContext::FromModelType(layer.m_type),
                            TokenRange(curToken, curToken + n));
      if (IsLayerSequenceSane(layers))
        MatchPOIsAndBuildings(ctx, curToken + n);
    }
  }
}

bool Geocoder::IsLayerSequenceSane(vector<FeaturesLayer> const & layers) const
{
  ASSERT(!layers.empty(), ());
  static_assert(Model::TYPE_COUNT <= 32, "Select a wider type to represent search types mask.");
  uint32_t mask = 0;
  size_t buildingIndex = layers.size();
  size_t streetIndex = layers.size();

  // Following loop returns false iff there're two different layers
  // of the same search type.
  for (size_t i = 0; i < layers.size(); ++i)
  {
    auto const & layer = layers[i];
    ASSERT_NOT_EQUAL(layer.m_type, Model::TYPE_COUNT, ());

    // TODO (@y): probably it's worth to check belongs-to-locality here.
    uint32_t bit = 1U << layer.m_type;
    if (mask & bit)
      return false;
    mask |= bit;

    if (layer.m_type == Model::TYPE_BUILDING)
      buildingIndex = i;
    else if (layer.m_type == Model::TYPE_STREET)
      streetIndex = i;
  }

  bool const hasBuildings = buildingIndex != layers.size();
  bool const hasStreets = streetIndex != layers.size();

  // Checks that building and street layers are neighbours.
  if (hasBuildings && hasStreets)
  {
    auto const & buildings = layers[buildingIndex];
    auto const & streets = layers[streetIndex];
    if (!buildings.m_tokenRange.AdjacentTo(streets.m_tokenRange))
      return false;
  }

  return true;
}

void Geocoder::FindPaths(BaseContext & ctx)
{
  auto const & layers = ctx.m_layers;

  if (layers.empty())
    return;

  // Layers ordered by search type.
  vector<FeaturesLayer const *> sortedLayers;
  sortedLayers.reserve(layers.size());
  for (auto const & layer : layers)
    sortedLayers.push_back(&layer);
  sort(sortedLayers.begin(), sortedLayers.end(), base::LessBy(&FeaturesLayer::m_type));

  auto const & innermostLayer = *sortedLayers.front();

  if (!m_postcodes.m_features.IsEmpty())
    m_matcher->SetPostcodes(&m_postcodes.m_features);
  else
    m_matcher->SetPostcodes(nullptr);

  m_finder.ForEachReachableVertex(
      *m_matcher, sortedLayers, [this, &ctx, &innermostLayer](IntersectionResult const & result) {
        ASSERT(result.IsValid(), ());
        EmitResult(ctx, m_context->GetId(), result.InnermostResult(), innermostLayer.m_type,
                   innermostLayer.m_tokenRange, &result, ctx.AllTokensUsed());
      });
}

void Geocoder::TraceResult(Tracer & tracer, BaseContext const & ctx, MwmSet::MwmId const & mwmId,
                           uint32_t ftId, Model::Type type, TokenRange const & tokenRange)
{
  SCOPE_GUARD(emitParse, [&]() { tracer.EmitParse(ctx.m_tokens); });

  if (type != Model::TYPE_POI && type != Model::TYPE_BUILDING)
    return;

  if (mwmId != m_context->GetId())
    return;

  FeatureType ft;
  if (!m_context->GetFeature(ftId, ft))
    return;

  feature::TypesHolder holder(ft);
  CategoriesInfo catInfo(holder, TokenSlice(m_params, tokenRange), m_params.m_categoryLocales,
                         m_categories);

  emitParse.release();
  tracer.EmitParse(ctx.m_tokens, catInfo.IsPureCategories());
}

void Geocoder::EmitResult(BaseContext & ctx, MwmSet::MwmId const & mwmId, uint32_t ftId,
                          Model::Type type, TokenRange const & tokenRange,
                          IntersectionResult const * geoParts, bool allTokensUsed)
{
  FeatureID id(mwmId, ftId);

  if (ctx.m_hotelsFilter && !ctx.m_hotelsFilter->Matches(id))
    return;

  if (ctx.m_cuisineFilter && !ctx.m_cuisineFilter->Matches(id))
    return;

  if (m_params.m_tracer)
    TraceResult(*m_params.m_tracer, ctx, mwmId, ftId, type, tokenRange);

  // Distance and rank will be filled at the end, for all results at once.
  //
  // TODO (@y, @m): need to skip zero rank features that are too
  // distant from the pivot when there're enough results close to the
  // pivot.
  PreRankingInfo info(type, tokenRange);
  for (auto const & layer : ctx.m_layers)
    info.m_tokenRange[layer.m_type] = layer.m_tokenRange;

  for (auto const * region : ctx.m_regions)
  {
    auto const regionType = Region::ToModelType(region->m_type);
    ASSERT_NOT_EQUAL(regionType, Model::TYPE_COUNT, ());
    info.m_tokenRange[regionType] = region->m_tokenRange;
  }

  if (ctx.m_city)
  {
    auto const & city = *ctx.m_city;
    info.m_tokenRange[Model::TYPE_CITY] = city.m_tokenRange;
    info.m_cityId = FeatureID(city.m_countryId, city.m_featureId);
  }

  if (geoParts)
    info.m_geoParts = *geoParts;

  info.m_allTokensUsed = allTokensUsed;

  m_preRanker.Emplace(id, info);

  // ++ctx.m_numEmitted;
}

void Geocoder::EmitResult(BaseContext & ctx, Region const & region, TokenRange const & tokenRange,
                          bool allTokensUsed)
{
  auto const type = Region::ToModelType(region.m_type);
  EmitResult(ctx, region.m_countryId, region.m_featureId, type, tokenRange, nullptr /* geoParts */,
             allTokensUsed);
}

void Geocoder::EmitResult(BaseContext & ctx, City const & city, TokenRange const & tokenRange,
                          bool allTokensUsed)
{
  EmitResult(ctx, city.m_countryId, city.m_featureId, city.m_type, tokenRange,
             nullptr /* geoParts */, allTokensUsed);
}

void Geocoder::MatchUnclassified(BaseContext & ctx, size_t curToken)
{
  ASSERT(ctx.m_layers.empty(), ());

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
       curToken < ctx.m_numTokens && !ctx.IsTokenUsed(curToken); ++curToken)
  {
    allFeatures = allFeatures.Intersect(ctx.m_features[curToken]);
  }

  if (m_filter->NeedToFilter(allFeatures))
    allFeatures = m_filter->Filter(allFeatures);

  auto emitUnclassified = [&](uint64_t bit)
  {
    auto const featureId = base::asserted_cast<uint32_t>(bit);
    Model::Type type;
    if (!GetTypeInGeocoding(ctx, featureId, type))
      return;
    if (type == Model::TYPE_UNCLASSIFIED)
    {
      EmitResult(ctx, m_context->GetId(), featureId, type, TokenRange(startToken, curToken),
                 nullptr /* geoParts */, true /* allTokensUsed */);
    }
  };
  allFeatures.ForEach(emitUnclassified);
}

CBV Geocoder::RetrievePostcodeFeatures(MwmContext const & context, TokenSlice const & slice)
{
  Retrieval retrieval(context, m_cancellable);
  return CBV(retrieval.RetrievePostcodeFeatures(slice));
}

CBV Geocoder::RetrieveGeometryFeatures(MwmContext const & context, m2::RectD const & rect,
                                       RectId id)
{
  switch (id)
  {
  case RECT_ID_PIVOT: return m_pivotRectsCache.Get(context, rect, m_params.GetScale());
  case RECT_ID_LOCALITY: return m_localityRectsCache.Get(context, rect, m_params.GetScale());
  case RECT_ID_COUNT: ASSERT(false, ("Invalid RectId.")); return CBV();
  }
  UNREACHABLE();
}

bool Geocoder::GetTypeInGeocoding(BaseContext const & ctx, uint32_t featureId, Model::Type & type)
{
  if (ctx.m_streets.HasBit(featureId))
  {
    type = Model::TYPE_STREET;
    return true;
  }
  if (ctx.m_villages.HasBit(featureId))
  {
    type = Model::TYPE_VILLAGE;
    return true;
  }

  FeatureType feature;
  if (m_context->GetFeature(featureId, feature))
  {
    type = m_model.GetType(feature);
    return true;
  }

  return false;
}
}  // namespace search
