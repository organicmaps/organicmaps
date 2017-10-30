#include "processor.hpp"

#include "search/common.hpp"
#include "search/dummy_rank_table.hpp"
#include "search/geometry_utils.hpp"
#include "search/intermediate_result.hpp"
#include "search/latlon_match.hpp"
#include "search/pre_ranking_info.hpp"
#include "search/query_params.hpp"
#include "search/ranking_info.hpp"
#include "search/ranking_utils.hpp"
#include "search/search_index_values.hpp"
#include "search/utils.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/index.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_covering.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/index.hpp"
#include "indexer/scales.hpp"
#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"
#include "indexer/trie_reader.hpp"

#include "geometry/mercator.hpp"

#include "platform/mwm_traits.hpp"
#include "platform/mwm_version.hpp"
#include "platform/preferred_languages.hpp"

#include "coding/compressed_bit_vector.hpp"
#include "coding/multilang_utf8_string.hpp"
#include "coding/reader_wrapper.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_add.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/function.hpp"
#include "std/iterator.hpp"
#include "std/limits.hpp"

#include "3party/Alohalytics/src/alohalytics.h"

namespace search
{
namespace
{
/// This indexes should match the initialization routine below.
int const g_arrLang1[] = {0, 1, 2, 2, 3};
int const g_arrLang2[] = {0, 0, 0, 1, 0};

enum LangIndexT
{
  LANG_CURRENT = 0,
  LANG_INPUT,
  LANG_INTERNATIONAL,
  LANG_EN,
  LANG_DEFAULT,
  LANG_COUNT
};

pair<int, int> GetLangIndex(int id)
{
  ASSERT_LESS(id, LANG_COUNT, ());
  return make_pair(g_arrLang1[id], g_arrLang2[id]);
}

m2::RectD NormalizeViewport(m2::RectD viewport)
{
  m2::RectD minViewport = MercatorBounds::RectByCenterXYAndSizeInMeters(
      viewport.Center(), Processor::kMinViewportRadiusM);
  viewport.Add(minViewport);

  m2::RectD maxViewport = MercatorBounds::RectByCenterXYAndSizeInMeters(
      viewport.Center(), Processor::kMaxViewportRadiusM);
  VERIFY(viewport.Intersect(maxViewport), ());
  return viewport;
}

m2::RectD GetRectAroundPosition(m2::PointD const & position)
{
  double constexpr kMaxPositionRadiusM = 50.0 * 1000;
  return MercatorBounds::RectByCenterXYAndSizeInMeters(position, kMaxPositionRadiusM);
}

void SendStatistics(SearchParams const & params, m2::RectD const & viewport, Results const & res)
{
  size_t const kMaxNumResultsToSend = 10;

  size_t const numResultsToSend = min(kMaxNumResultsToSend, res.GetCount());
  string resultString = strings::to_string(numResultsToSend);
  for (size_t i = 0; i < numResultsToSend; ++i)
    resultString.append("\t" + res[i].ToStringForStats());

  string posX, posY;
  if (params.IsValidPosition())
  {
    auto const position = params.GetPositionMercator();
    posX = strings::to_string(position.x);
    posY = strings::to_string(position.y);
  }

  alohalytics::TStringMap const stats = {
      {"posX", posX},
      {"posY", posY},
      {"viewportMinX", strings::to_string(viewport.minX())},
      {"viewportMinY", strings::to_string(viewport.minY())},
      {"viewportMaxX", strings::to_string(viewport.maxX())},
      {"viewportMaxY", strings::to_string(viewport.maxY())},
      {"query", params.m_query},
      {"locale", params.m_inputLocale},
      {"results", resultString},
  };
  alohalytics::LogEvent("searchEmitResultsAndCoords", stats);
  GetPlatform().GetMarketingService().SendMarketingEvent(marketing::kSearchEmitResultsAndCoords, {});
}

// Removes all full-token stop words from |params|.
// Does nothing if all tokens in |params| are non-prefix stop words.
void RemoveStopWordsIfNeeded(QueryParams & params)
{
  size_t numStopWords = 0;
  for (size_t i = 0; i < params.GetNumTokens(); ++i)
  {
    auto & token = params.GetToken(i);
    if (!params.IsPrefixToken(i) && IsStopWord(token.m_original))
      ++numStopWords;
  }

  if (numStopWords == params.GetNumTokens())
    return;

  for (size_t i = 0; i < params.GetNumTokens();)
  {
    if (params.IsPrefixToken(i))
    {
      ++i;
      continue;
    }

    auto & token = params.GetToken(i);
    if (IsStopWord(token.m_original))
    {
      params.RemoveToken(i);
    }
    else
    {
      my::EraseIf(token.m_synonyms, &IsStopWord);
      ++i;
    }
  }
}
}  // namespace

// static
size_t const Processor::kPreResultsCount = 200;

// static
double const Processor::kMinViewportRadiusM = 5.0 * 1000;
double const Processor::kMaxViewportRadiusM = 50.0 * 1000;

Processor::Processor(Index const & index, CategoriesHolder const & categories,
                     vector<Suggest> const & suggests,
                     storage::CountryInfoGetter const & infoGetter)
  : m_categories(categories)
  , m_infoGetter(infoGetter)
  , m_position(0, 0)
  , m_minDistanceOnMapBetweenResults(0.0)
  , m_mode(Mode::Everywhere)
  , m_suggestsEnabled(true)
  , m_viewportSearch(false)
  , m_villagesCache(static_cast<my::Cancellable const &>(*this))
  , m_citiesBoundaries(index)
  , m_ranker(index, m_citiesBoundaries, infoGetter, m_keywordsScorer, m_emitter, categories,
             suggests, m_villagesCache, static_cast<my::Cancellable const &>(*this))
  , m_preRanker(index, m_ranker, kPreResultsCount)
  , m_geocoder(index, infoGetter, m_preRanker, m_villagesCache,
               static_cast<my::Cancellable const &>(*this))
{
  // Initialize keywords scorer.
  // Note! This order should match the indexes arrays above.
  vector<vector<int8_t>> langPriorities = {
      {-1},  // future current lang
      {-1},  // future input lang
      {StringUtf8Multilang::kInternationalCode, StringUtf8Multilang::kEnglishCode},
      {StringUtf8Multilang::kDefaultCode}};

  m_keywordsScorer.SetLanguages(langPriorities);

  SetPreferredLocale("en");
}

void Processor::Init(bool viewportSearch)
{
  m_tokens.clear();
  m_prefix.clear();
  m_preRanker.SetViewportSearch(viewportSearch);
}

void Processor::SetViewport(m2::RectD const & viewport)
{
  ASSERT(viewport.IsValid(), ());

  if (m_viewport.IsValid())
  {
    double constexpr kEpsMeters = 10.0;
    if (IsEqualMercator(m_viewport, viewport, kEpsMeters))
      return;
  }

  m_viewport = viewport;
}

void Processor::SetPreferredLocale(string const & locale)
{
  ASSERT(!locale.empty(), ());

  LOG(LINFO, ("New preferred locale:", locale));

  int8_t const code = StringUtf8Multilang::GetLangIndex(languages::Normalize(locale));
  SetLanguage(LANG_CURRENT, code);

  m_currentLocaleCode = CategoriesHolder::MapLocaleToInteger(locale);

  // Default initialization.
  // If you want to reset input language, call SetInputLocale before search.
  SetInputLocale(locale);
  m_ranker.SetLocalityLanguage(code);
}

void Processor::SetInputLocale(string const & locale)
{
  if (locale.empty())
    return;

  LOG(LDEBUG, ("New input locale:", locale));
  SetLanguage(LANG_INPUT, StringUtf8Multilang::GetLangIndex(languages::Normalize(locale)));
  m_inputLocaleCode = CategoriesHolder::MapLocaleToInteger(locale);
}

void Processor::SetQuery(string const & query)
{
  m_query = query;

  /// @todo Why Init is separated with SetQuery?
  ASSERT(m_tokens.empty(), ());
  ASSERT(m_prefix.empty(), ());

  // Following code splits input query by delimiters except hash tags
  // first, and then splits result tokens by hashtags. The goal is to
  // retrieve all tokens that start with a single hashtag and leave
  // them as is.

  vector<strings::UniString> tokens;
  {
    search::DelimitersWithExceptions delims(vector<strings::UniChar>{'#'});
    SplitUniString(NormalizeAndSimplifyString(query), MakeBackInsertFunctor(tokens), delims);
  }

  search::Delimiters delims;
  {
    QueryTokens subTokens;
    for (auto const & token : tokens)
    {
      size_t numHashes = 0;
      for (; numHashes < token.size() && token[numHashes] == '#'; ++numHashes)
        ;

      // Splits |token| by hashtags, because all other delimiters are
      // already removed.
      subTokens.clear();
      SplitUniString(token, MakeBackInsertFunctor(subTokens), delims);
      if (subTokens.empty())
        continue;

      if (numHashes == 1)
        m_tokens.push_back(strings::MakeUniString("#") + subTokens[0]);
      else
        m_tokens.emplace_back(move(subTokens[0]));

      for (size_t i = 1; i < subTokens.size(); ++i)
        m_tokens.push_back(move(subTokens[i]));
    }
  }

  // Assign prefix with last parsed token.
  if (!m_tokens.empty() && !delims(strings::LastUniChar(query)))
  {
    m_prefix.swap(m_tokens.back());
    m_tokens.pop_back();
  }

  int const maxTokensCount = MAX_TOKENS - 1;
  if (m_tokens.size() > maxTokensCount)
    m_tokens.resize(maxTokensCount);

  // Assign tokens and prefix to scorer.
  m_keywordsScorer.SetKeywords(m_tokens.data(), m_tokens.size(), m_prefix);

  // Get preferred types to show in results.
  m_preferredTypes.clear();
  ForEachCategoryType(QuerySliceOnRawStrings<decltype(m_tokens)>(m_tokens, m_prefix),
                      [&](size_t, uint32_t t)
                      {
                        m_preferredTypes.insert(t);
                      });
}

void Processor::SetRankPivot(m2::PointD const & pivot)
{
  if (!m2::AlmostEqualULPs(pivot, m_pivot))
  {
    storage::CountryInfo ci;
    m_infoGetter.GetRegionInfo(pivot, ci);
    m_region.swap(ci.m_name);
  }

  m_pivot = pivot;
}

void Processor::SetLanguage(int id, int8_t lang)
{
  m_keywordsScorer.SetLanguage(GetLangIndex(id), lang);
}

int8_t Processor::GetLanguage(int id) const
{
  return m_keywordsScorer.GetLanguage(GetLangIndex(id));
}

m2::PointD Processor::GetPivotPoint() const
{
  bool const viewportSearch = m_mode == Mode::Viewport;

  auto const & viewport = GetViewport();
  if (viewportSearch || !viewport.IsPointInside(GetPosition()))
    return viewport.Center();
  return GetPosition();
}

m2::RectD Processor::GetPivotRect() const
{
  auto const & viewport = GetViewport();
  if (viewport.IsPointInside(GetPosition()))
    return GetRectAroundPosition(GetPosition());
  return NormalizeViewport(viewport);
}

m2::RectD const & Processor::GetViewport() const
{
  ASSERT(m_viewport.IsValid(), ());
  return m_viewport;
}

void Processor::LoadCitiesBoundaries()
{
  if (m_citiesBoundaries.Load())
    LOG(LINFO, ("Loaded cities boundaries"));
  else
    LOG(LWARNING, ("Can't load cities boundaries"));
}

Locales Processor::GetCategoryLocales() const
{
  static int8_t const enLocaleCode = CategoriesHolder::MapLocaleToInteger("en");
  Locales result;

  // Prepare array of processing locales. English locale is always present for category matching.
  result.Insert(static_cast<uint64_t>(enLocaleCode));
  if (m_currentLocaleCode != -1)
    result.Insert(static_cast<uint64_t>(m_currentLocaleCode));
  if (m_inputLocaleCode != -1)
    result.Insert(static_cast<uint64_t>(m_inputLocaleCode));

  return result;
}

template <typename ToDo>
void Processor::ForEachCategoryType(StringSliceBase const & slice, ToDo && toDo) const
{
  ::search::ForEachCategoryType(slice, GetCategoryLocales(), m_categories, forward<ToDo>(toDo));
}

template <typename ToDo>
void Processor::ForEachCategoryTypeFuzzy(StringSliceBase const & slice, ToDo && toDo) const
{
  ::search::ForEachCategoryTypeFuzzy(slice, GetCategoryLocales(), m_categories,
                                     forward<ToDo>(toDo));
}

void Processor::Search(SearchParams const & params)
{
  if (params.m_onStarted)
    params.m_onStarted();

  if (IsCancelled())
  {
    Results results;
    results.SetEndMarker(true /* isCancelled */);

    if (params.m_onResults)
      params.m_onResults(results);
    else
      LOG(LERROR, ("OnResults is not set."));
    return;
  }

  SetMode(params.m_mode);
  bool const viewportSearch = m_mode == Mode::Viewport;

  bool rankPivotIsSet = false;
  auto const & viewport = params.m_viewport;
  ASSERT(viewport.IsValid(), ());

  if (!viewportSearch && params.IsValidPosition())
  {
    m2::PointD const pos = params.GetPositionMercator();
    if (m2::Inflate(viewport, viewport.SizeX() / 4.0, viewport.SizeY() / 4.0).IsPointInside(pos))
    {
      SetRankPivot(pos);
      rankPivotIsSet = true;
    }
  }
  if (!rankPivotIsSet)
    SetRankPivot(viewport.Center());

  if (params.IsValidPosition())
    SetPosition(params.GetPositionMercator());
  else
    SetPosition(viewport.Center());

  SetMinDistanceOnMapBetweenResults(params.m_minDistanceOnMapBetweenResults);

  SetSuggestsEnabled(params.m_suggestsEnabled);
  m_hotelsFilter = params.m_hotelsFilter;
  m_cianMode = params.m_cianMode;

  SetInputLocale(params.m_inputLocale);

  SetQuery(params.m_query);
  SetViewport(viewport);
  SetOnResults(params.m_onResults);

  Geocoder::Params geocoderParams;
  InitGeocoder(geocoderParams);
  InitPreRanker(geocoderParams);
  InitRanker(geocoderParams);
  InitEmitter();

  try
  {
    SearchCoordinates();

    if (viewportSearch)
    {
      m_geocoder.GoInViewport();
    }
    else
    {
      if (m_tokens.empty())
        m_ranker.SuggestStrings();

      m_geocoder.GoEverywhere();
    }

    m_ranker.UpdateResults(true /* lastUpdate */);
  }
  catch (CancelException const &)
  {
    LOG(LDEBUG, ("Search has been cancelled."));
  }

  if (!viewportSearch && !IsCancelled())
    SendStatistics(params, viewport, m_emitter.GetResults());

  // Emit finish marker to client.
  m_emitter.Finish(IsCancelled());
}

void Processor::SearchCoordinates()
{
  double lat, lon;
  if (!MatchLatLonDegree(m_query, lat, lon))
    return;
  m_emitter.AddResultNoChecks(m_ranker.MakeResult(RankerResult(lat, lon), true /* needAddress */,
                                                  true /* needHighlighting */));
  m_emitter.Emit();
}

void Processor::InitParams(QueryParams & params)
{
  if (m_prefix.empty())
    params.InitNoPrefix(m_tokens.begin(), m_tokens.end());
  else
    params.InitWithPrefix(m_tokens.begin(), m_tokens.end(), m_prefix);

  RemoveStopWordsIfNeeded(params);

  // Add names of categories (and synonyms).
  Classificator const & c = classif();
  auto addSynonyms = [&](size_t i, uint32_t t) {
    uint32_t const index = c.GetIndexForType(t);
    params.GetTypeIndices(i).push_back(index);
  };
  auto const tokenSlice = QuerySliceOnRawStrings<decltype(m_tokens)>(m_tokens, m_prefix);
  bool const isCategorialRequest =
      IsCategorialRequest(tokenSlice, GetCategoryLocales(), m_categories);
  params.SetCategorialRequest(isCategorialRequest);
  if (isCategorialRequest)
  {
    ForEachCategoryType(tokenSlice, addSynonyms);
  }
  else
  {
    // todo(@m, @y). Shall we match prefix tokens for categories?
    ForEachCategoryTypeFuzzy(tokenSlice, addSynonyms);
  }

  // Remove all type indices for streets, as they're considired
  // individually.
  for (size_t i = 0; i < params.GetNumTokens(); ++i)
  {
    auto & token = params.GetToken(i);
    if (IsStreetSynonym(token.m_original))
      params.GetTypeIndices(i).clear();
  }

  for (size_t i = 0; i < params.GetNumTokens(); ++i)
    my::SortUnique(params.GetTypeIndices(i));

  for (int i = 0; i < LANG_COUNT; ++i)
    params.GetLangs().Insert(GetLanguage(i));
}

void Processor::InitGeocoder(Geocoder::Params & params)
{
  bool const viewportSearch = m_mode == Mode::Viewport;

  InitParams(params);
  params.m_mode = m_mode;
  if (viewportSearch)
    params.m_pivot = GetViewport();
  else
    params.m_pivot = GetPivotRect();
  params.m_hotelsFilter = m_hotelsFilter;
  params.m_cianMode = m_cianMode;
  params.m_preferredTypes = m_preferredTypes;

  m_geocoder.SetParams(params);
}

void Processor::InitPreRanker(Geocoder::Params const & geocoderParams)
{
  bool const viewportSearch = m_mode == Mode::Viewport;

  PreRanker::Params params;

  if (viewportSearch)
  {
    params.m_viewport = GetViewport();
    params.m_minDistanceOnMapBetweenResults = m_minDistanceOnMapBetweenResults;
  }
  params.m_accuratePivotCenter = GetPivotPoint();
  params.m_scale = geocoderParams.GetScale();

  m_preRanker.Init(params);
}

void Processor::InitRanker(Geocoder::Params const & geocoderParams)
{
  size_t const kResultsCount = 30;
  bool const viewportSearch = m_mode == Mode::Viewport;
  Ranker::Params params;

  params.m_currentLocaleCode = m_currentLocaleCode;
  if (viewportSearch)
  {
    params.m_viewport = GetViewport();
    params.m_minDistanceOnMapBetweenResults = m_minDistanceOnMapBetweenResults;
    params.m_limit = kPreResultsCount;
  }
  else
  {
    params.m_minDistanceOnMapBetweenResults = 100.0;
    params.m_limit = kResultsCount;
  }
  params.m_position = GetPosition();
  params.m_pivotRegion = GetPivotRegion();
  params.m_preferredTypes = m_preferredTypes;
  params.m_suggestsEnabled = m_suggestsEnabled;
  params.m_query = m_query;
  params.m_tokens = m_tokens;
  params.m_prefix = m_prefix;
  params.m_categoryLocales = GetCategoryLocales();
  params.m_accuratePivotCenter = GetPivotPoint();
  params.m_viewportSearch = viewportSearch;
  m_ranker.Init(params, geocoderParams);
}

void Processor::InitEmitter() { m_emitter.Init(m_onResults); }

void Processor::ClearCaches()
{
  m_geocoder.ClearCaches();
  m_villagesCache.Clear();
  m_preRanker.ClearCaches();
  m_ranker.ClearCaches();
  m_viewport.MakeEmpty();
}
}  // namespace search
