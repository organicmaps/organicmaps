#include "processor.hpp"

#include "search/common.hpp"
#include "search/dummy_rank_table.hpp"
#include "search/geometry_utils.hpp"
#include "search/intermediate_result.hpp"
#include "search/latlon_match.hpp"
#include "search/mode.hpp"
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
enum LanguageTier
{
  LANGUAGE_TIER_CURRENT = 0,
  LANGUAGE_TIER_INPUT,
  LANGUAGE_TIER_EN_AND_INTERNATIONAL,
  LANGUAGE_TIER_DEFAULT,
  LANGUAGE_TIER_COUNT
};

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
  if (params.m_position)
  {
    auto const position = *params.m_position;
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
double const Processor::kMinViewportRadiusM = 5.0 * 1000;
double const Processor::kMaxViewportRadiusM = 50.0 * 1000;
double const Processor::kMinDistanceOnMapBetweenResultsM = 100.0;

Processor::Processor(Index const & index, CategoriesHolder const & categories,
                     vector<Suggest> const & suggests,
                     storage::CountryInfoGetter const & infoGetter)
  : m_categories(categories)
  , m_infoGetter(infoGetter)
  , m_position(0, 0)
  , m_villagesCache(static_cast<my::Cancellable const &>(*this))
  , m_citiesBoundaries(index)
  , m_keywordsScorer(LanguageTier::LANGUAGE_TIER_COUNT)
  , m_ranker(index, m_citiesBoundaries, infoGetter, m_keywordsScorer, m_emitter, categories,
             suggests, m_villagesCache, static_cast<my::Cancellable const &>(*this))
  , m_preRanker(index, m_ranker)
  , m_geocoder(index, infoGetter, m_preRanker, m_villagesCache,
               static_cast<my::Cancellable const &>(*this))
{
  // Current and input langs are to be set later.
  m_keywordsScorer.SetLanguages(
      LanguageTier::LANGUAGE_TIER_EN_AND_INTERNATIONAL,
      {StringUtf8Multilang::kInternationalCode, StringUtf8Multilang::kEnglishCode});
  m_keywordsScorer.SetLanguages(LanguageTier::LANGUAGE_TIER_DEFAULT,
                                {StringUtf8Multilang::kDefaultCode});

  SetPreferredLocale("en");
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
  m_keywordsScorer.SetLanguages(LanguageTier::LANGUAGE_TIER_CURRENT, {code});

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
  int8_t const code = StringUtf8Multilang::GetLangIndex(languages::Normalize(locale));
  m_keywordsScorer.SetLanguages(LanguageTier::LANGUAGE_TIER_INPUT, {code});
  m_inputLocaleCode = CategoriesHolder::MapLocaleToInteger(locale);
}

void Processor::SetQuery(string const & query)
{
  m_query = query;
  m_tokens.clear();
  m_prefix.clear();

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

m2::PointD Processor::GetPivotPoint(bool viewportSearch) const
{
  auto const & viewport = GetViewport();
  if (viewportSearch || !viewport.IsPointInside(GetPosition()))
    return viewport.Center();
  return GetPosition();
}

m2::RectD Processor::GetPivotRect(bool viewportSearch) const
{
  auto const & viewport = GetViewport();

  if (viewportSearch)
    return viewport;

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

  bool const viewportSearch = params.m_mode == Mode::Viewport;

  auto const & viewport = params.m_viewport;
  ASSERT(viewport.IsValid(), ());

  if (params.m_position)
    SetPosition(*params.m_position);
  else
    SetPosition(viewport.Center());

  SetInputLocale(params.m_inputLocale);

  SetQuery(params.m_query);
  SetViewport(viewport);

  Geocoder::Params geocoderParams;
  InitGeocoder(geocoderParams, params);
  InitPreRanker(geocoderParams, params);
  InitRanker(geocoderParams, params);
  InitEmitter(params);

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

  m_keywordsScorer.ForEachLanguage(
      [&](int8_t lang) { params.GetLangs().Insert(static_cast<uint64_t>(lang)); });
}

void Processor::InitGeocoder(Geocoder::Params & geocoderParams, SearchParams const & searchParams)
{
  auto const viewportSearch = searchParams.m_mode == Mode::Viewport;

  InitParams(geocoderParams);

  geocoderParams.m_mode = searchParams.m_mode;
  geocoderParams.m_pivot = GetPivotRect(viewportSearch);
  geocoderParams.m_hotelsFilter = searchParams.m_hotelsFilter;
  geocoderParams.m_cianMode = searchParams.m_cianMode;
  geocoderParams.m_preferredTypes = m_preferredTypes;
  geocoderParams.m_tracer = searchParams.m_tracer;

  m_geocoder.SetParams(geocoderParams);
}

void Processor::InitPreRanker(Geocoder::Params const & geocoderParams,
                              SearchParams const & searchParams)
{
  bool const viewportSearch = searchParams.m_mode == Mode::Viewport;

  PreRanker::Params params;

  if (viewportSearch)
  {
    params.m_viewport = GetViewport();
    params.m_minDistanceOnMapBetweenResults =
        searchParams.m_minDistanceOnMapBetweenResults;
  }
  params.m_accuratePivotCenter = GetPivotPoint(viewportSearch);
  params.m_scale = geocoderParams.GetScale();
  params.m_limit = max(kPreResultsCount, searchParams.m_maxNumResults);
  params.m_viewportSearch = viewportSearch;

  m_preRanker.Init(params);
}

void Processor::InitRanker(Geocoder::Params const & geocoderParams,
                           SearchParams const & searchParams)
{
  bool const viewportSearch = searchParams.m_mode == Mode::Viewport;

  Ranker::Params params;

  params.m_currentLocaleCode = m_currentLocaleCode;

  if (viewportSearch)
  {
    params.m_viewport = GetViewport();
    params.m_minDistanceOnMapBetweenResults = searchParams.m_minDistanceOnMapBetweenResults;
  }
  else
  {
    params.m_minDistanceOnMapBetweenResults = kMinDistanceOnMapBetweenResultsM;
  }

  params.m_limit = searchParams.m_maxNumResults;
  params.m_position = GetPosition();
  params.m_pivotRegion = GetPivotRegion();
  params.m_preferredTypes = m_preferredTypes;
  params.m_suggestsEnabled = searchParams.m_suggestsEnabled;
  params.m_needAddress = searchParams.m_needAddress;
  params.m_needHighlighting = searchParams.m_needHighlighting;
  params.m_query = m_query;
  params.m_tokens = m_tokens;
  params.m_prefix = m_prefix;
  params.m_categoryLocales = GetCategoryLocales();
  params.m_accuratePivotCenter = GetPivotPoint(viewportSearch);
  params.m_viewportSearch = viewportSearch;

  m_ranker.Init(params, geocoderParams);
}

void Processor::InitEmitter(SearchParams const & searchParams)
{
  m_emitter.Init(searchParams.m_onResults);
}

void Processor::ClearCaches()
{
  m_geocoder.ClearCaches();
  m_villagesCache.Clear();
  m_preRanker.ClearCaches();
  m_ranker.ClearCaches();
  m_viewport.MakeEmpty();
}
}  // namespace search
