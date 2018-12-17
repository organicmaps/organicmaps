#include "processor.hpp"

#include "search/common.hpp"
#include "search/cuisine_filter.hpp"
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
#include "indexer/data_source.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_covering.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"
#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"
#include "indexer/trie_reader.hpp"

#include "geometry/mercator.hpp"

#include "platform/mwm_traits.hpp"
#include "platform/mwm_version.hpp"
#include "platform/preferred_languages.hpp"

#include "coding/compressed_bit_vector.hpp"
#include "coding/reader_wrapper.hpp"
#include "coding/string_utf8_multilang.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>

#include "3party/Alohalytics/src/alohalytics.h"
#include "3party/open-location-code/openlocationcode.h"

using namespace std;

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

// Removes all full-token stop words from |tokens|.
// Does nothing if all tokens are non-prefix stop words.
void RemoveStopWordsIfNeeded(QueryTokens & tokens, strings::UniString & prefix)
{
  size_t numStopWords = 0;
  for (auto const & token : tokens)
  {
    if (IsStopWord(token))
      ++numStopWords;
  }

  if (numStopWords == tokens.size() && prefix.empty())
    return;

  tokens.erase_if(&IsStopWord);
}
}  // namespace

// static
size_t const Processor::kPreResultsCount = 200;

Processor::Processor(DataSource const & dataSource, CategoriesHolder const & categories,
                     vector<Suggest> const & suggests,
                     storage::CountryInfoGetter const & infoGetter)
  : m_categories(categories)
  , m_infoGetter(infoGetter)
  , m_villagesCache(static_cast<base::Cancellable const &>(*this))
  , m_citiesBoundaries(dataSource)
  , m_keywordsScorer(LanguageTier::LANGUAGE_TIER_COUNT)
  , m_ranker(dataSource, m_citiesBoundaries, infoGetter, m_keywordsScorer, m_emitter, categories,
             suggests, m_villagesCache, static_cast<base::Cancellable const &>(*this))
  , m_preRanker(dataSource, m_ranker)
  , m_geocoder(dataSource, infoGetter, categories, m_citiesBoundaries, m_preRanker, m_villagesCache,
               static_cast<base::Cancellable const &>(*this))
  , m_bookmarksProcessor(m_emitter, static_cast<base::Cancellable const &>(*this))
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
    double const kEps = MercatorBounds::MetersToMercator(kEpsMeters);
    if (IsEqualMercator(m_viewport, viewport, kEps))
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
  m_ranker.SetLocale(locale);
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
    SplitUniString(NormalizeAndSimplifyString(query), base::MakeBackInsertFunctor(tokens), delims);
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
      SplitUniString(token, base::MakeBackInsertFunctor(subTokens), delims);
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

  static_assert(kMaxNumTokens > 0, "");
  size_t const maxTokensCount = kMaxNumTokens - 1;
  if (m_tokens.size() > maxTokensCount)
  {
    m_tokens.resize(maxTokensCount);
  }
  else
  {
    // Assign the last parsed token to prefix.
    if (!m_tokens.empty() && !delims(strings::LastUniChar(query)))
    {
      m_prefix.swap(m_tokens.back());
      m_tokens.pop_back();
    }
  }

  RemoveStopWordsIfNeeded(m_tokens, m_prefix);

  // Get preferred types to show in results.
  m_preferredTypes.clear();
  auto const tokenSlice = QuerySliceOnRawStrings<decltype(m_tokens)>(m_tokens, m_prefix);
  m_isCategorialRequest = FillCategories(tokenSlice, GetCategoryLocales(), m_categories, m_preferredTypes);

  // Try to match query to cuisine categories.
  if (!m_isCategorialRequest)
  {
    bool const isCuisineRequest = FillCategories(
        tokenSlice, GetCategoryLocales(), GetDefaultCuisineCategories(), m_cuisineTypes);

    if (isCuisineRequest)
    {
      m_isCategorialRequest = true;
      m_preferredTypes = ftypes::IsEatChecker::Instance().GetTypes();
    }

    // Assign tokens and prefix to scorer.
    m_keywordsScorer.SetKeywords(m_tokens.data(), m_tokens.size(), m_prefix);
  }

  if (!m_isCategorialRequest)
    ForEachCategoryType(tokenSlice, [&](size_t, uint32_t t) { m_preferredTypes.push_back(t); });

  base::SortUnique(m_preferredTypes);
}

m2::PointD Processor::GetPivotPoint(bool viewportSearch) const
{
  auto const & viewport = GetViewport();
  if (viewportSearch || !m_position || !viewport.IsPointInside(*m_position))
    return viewport.Center();

  CHECK(m_position, ());
  return *m_position;
}

m2::RectD Processor::GetPivotRect(bool viewportSearch) const
{
  auto const & viewport = GetViewport();

  if (viewportSearch || !m_position || !viewport.IsPointInside(*m_position))
    return viewport;

  CHECK(m_position, ());
  return GetRectAroundPosition(*m_position);
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

void Processor::LoadCountriesTree() { m_ranker.LoadCountriesTree(); }

void Processor::OnBookmarksCreated(vector<pair<bookmarks::Id, bookmarks::Doc>> const & marks)
{
  for (auto const & idDoc : marks)
    m_bookmarksProcessor.Add(idDoc.first /* id */, idDoc.second /* doc */);
}

void Processor::OnBookmarksUpdated(vector<pair<bookmarks::Id, bookmarks::Doc>> const & marks)
{
  for (auto const & idDoc : marks)
  {
    m_bookmarksProcessor.Erase(idDoc.first /* id */);
    m_bookmarksProcessor.Add(idDoc.first /* id */, idDoc.second /* doc */);
  }
}

void Processor::OnBookmarksDeleted(vector<bookmarks::Id> const & marks)
{
  for (auto const & id : marks)
    m_bookmarksProcessor.Erase(id);
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

  m_position = params.m_position;

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
    switch (params.m_mode)
    {
    case Mode::Everywhere:  // fallthrough
    case Mode::Viewport:    // fallthrough
    case Mode::Downloader:
      SearchCoordinates();
      SearchPlusCode();
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
      break;
    case Mode::Bookmarks: SearchBookmarks(); break;
    case Mode::Count: ASSERT(false, ("Invalid mode")); break;
    }
  }
  catch (CancelException const &)
  {
    LOG(LDEBUG, ("Search has been cancelled."));
  }

  if (!viewportSearch && !IsCancelled())
    SendStatistics(params, viewport, m_emitter.GetResults());

  // Emit finish marker to client.
  m_geocoder.Finish(IsCancelled());
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

void Processor::SearchPlusCode()
{
  // Create a copy of the query to trim it in-place.
  string query(m_query);
  strings::Trim(query);

  string code;

  if (openlocationcode::IsFull(query))
  {
    code = query;
  }
  else if (openlocationcode::IsShort(query))
  {
    if (!m_position)
      return;
    ms::LatLon const latLon = MercatorBounds::ToLatLon(*m_position);
    code = openlocationcode::RecoverNearest(query, {latLon.lat, latLon.lon});
  }

  if (code.empty())
    return;

  openlocationcode::CodeArea const area = openlocationcode::Decode(code);
  m_emitter.AddResultNoChecks(
      m_ranker.MakeResult(RankerResult(area.GetCenter().latitude, area.GetCenter().longitude),
                          true /* needAddress */, false /* needHighlighting */));
  m_emitter.Emit();
}

void Processor::SearchBookmarks() const
{
  QueryParams params;
  InitParams(params);
  m_bookmarksProcessor.Search(params);
}

void Processor::InitParams(QueryParams & params) const
{
  if (m_prefix.empty())
    params.InitNoPrefix(m_tokens.begin(), m_tokens.end());
  else
    params.InitWithPrefix(m_tokens.begin(), m_tokens.end(), m_prefix);

  // Add names of categories (and synonyms).
  Classificator const & c = classif();
  auto addCategorySynonyms = [&](size_t i, uint32_t t) {
    uint32_t const index = c.GetIndexForType(t);
    params.GetTypeIndices(i).push_back(index);
  };
  auto const tokenSlice = QuerySliceOnRawStrings<decltype(m_tokens)>(m_tokens, m_prefix);
  params.SetCategorialRequest(m_isCategorialRequest);
  if (m_isCategorialRequest)
  {
    for (auto const type : m_preferredTypes)
    {
      uint32_t const index = c.GetIndexForType(type);
      for (size_t i = 0; i < tokenSlice.Size(); ++i)
        params.GetTypeIndices(i).push_back(index);
    }
  }
  else
  {
    // todo(@m, @y). Shall we match prefix tokens for categories?
    ForEachCategoryTypeFuzzy(tokenSlice, addCategorySynonyms);
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
    base::SortUnique(params.GetTypeIndices(i));

  m_keywordsScorer.ForEachLanguage(
      [&](int8_t lang) { params.GetLangs().Insert(static_cast<uint64_t>(lang)); });
}

void Processor::InitGeocoder(Geocoder::Params & geocoderParams, SearchParams const & searchParams)
{
  auto const viewportSearch = searchParams.m_mode == Mode::Viewport;

  InitParams(geocoderParams);

  geocoderParams.m_mode = searchParams.m_mode;
  geocoderParams.m_pivot = GetPivotRect(viewportSearch);
  geocoderParams.m_position = m_position;
  geocoderParams.m_categoryLocales = GetCategoryLocales();
  geocoderParams.m_hotelsFilter = searchParams.m_hotelsFilter;
  geocoderParams.m_cuisineTypes = m_cuisineTypes;
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
    params.m_minDistanceOnMapBetweenResults = searchParams.m_minDistanceOnMapBetweenResults;

  params.m_viewport = GetViewport();
  params.m_accuratePivotCenter = GetPivotPoint(viewportSearch);
  params.m_position = m_position;
  params.m_scale = geocoderParams.GetScale();
  params.m_limit = max(kPreResultsCount, searchParams.m_maxNumResults);
  params.m_viewportSearch = viewportSearch;
  params.m_categorialRequest = geocoderParams.IsCategorialRequest();

  m_preRanker.Init(params);
}

void Processor::InitRanker(Geocoder::Params const & geocoderParams,
                           SearchParams const & searchParams)
{
  bool const viewportSearch = searchParams.m_mode == Mode::Viewport;

  Ranker::Params params;

  params.m_currentLocaleCode = m_currentLocaleCode;

  if (viewportSearch)
    params.m_viewport = GetViewport();

  params.m_limit = searchParams.m_maxNumResults;
  params.m_pivot = m_position ? *m_position : GetViewport().Center();
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
  params.m_categorialRequest = geocoderParams.IsCategorialRequest();

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
