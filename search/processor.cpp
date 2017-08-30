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
  , m_supportOldFormat(false)
  , m_viewportSearch(false)
  , m_villagesCache(static_cast<my::Cancellable const &>(*this))
  , m_ranker(index, infoGetter, m_emitter, categories, suggests, m_villagesCache,
             static_cast<my::Cancellable const &>(*this))
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

  m_ranker.SetLanguages(langPriorities);

  SetPreferredLocale("en");
}

void Processor::Init(bool viewportSearch)
{
  m_tokens.clear();
  m_prefix.clear();
  m_preRanker.SetViewportSearch(viewportSearch);
}

void Processor::SetViewport(m2::RectD const & viewport, bool forceUpdate)
{
  SetViewportByIndex(viewport, CURRENT_V, forceUpdate);
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
  m_ranker.SetLocalityFinderLanguage(code);
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
    buffer_vector<strings::UniString, 32> subTokens;
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
  m_ranker.SetKeywords(m_tokens.data(), m_tokens.size(), m_prefix);

  // get preferred types to show in results
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
  m_ranker.SetLanguage(GetLangIndex(id), lang);
}

int8_t Processor::GetLanguage(int id) const
{
  return m_ranker.GetLanguage(GetLangIndex(id));
}

m2::PointD Processor::GetPivotPoint() const
{
  bool const viewportSearch = m_mode == Mode::Viewport;

  m2::RectD const & viewport = m_viewport[CURRENT_V];
  if (viewportSearch || !viewport.IsPointInside(GetPosition()))
    return viewport.Center();
  return GetPosition();
}

m2::RectD Processor::GetPivotRect() const
{
  m2::RectD const & viewport = m_viewport[CURRENT_V];
  if (viewport.IsPointInside(GetPosition()))
    return GetRectAroundPosition(GetPosition());
  return NormalizeViewport(viewport);
}

void Processor::SetViewportByIndex(m2::RectD const & viewport, size_t idx, bool forceUpdate)
{
  ASSERT(idx < COUNT_V, (idx));

  if (viewport.IsValid())
  {
    // Check if we can skip this cache query.
    if (m_viewport[idx].IsValid())
    {
      // Threshold to compare for equal or inner rects.
      // It doesn't influence on result cached features because it's smaller
      // than minimal cell size in geometry index (i'm almost sure :)).
      double constexpr epsMeters = 10.0;

      if (forceUpdate)
      {
        // skip if rects are equal
        if (IsEqualMercator(m_viewport[idx], viewport, epsMeters))
          return;
      }
      else
      {
        // skip if the new viewport is inside the old one (no need to recache)
        m2::RectD r(m_viewport[idx]);
        double constexpr eps = epsMeters * MercatorBounds::degreeInMetres;
        r.Inflate(eps, eps);

        if (r.IsRectInside(viewport))
          return;
      }
    }

    m_viewport[idx] = viewport;
  }
  else
  {
    ClearCache(idx);
  }
}

void Processor::ClearCache(size_t ind) { m_viewport[ind].MakeEmpty(); }

TLocales Processor::GetCategoryLocales() const
{
  static int8_t const enLocaleCode = CategoriesHolder::MapLocaleToInteger("en");
  TLocales result;

  // Prepare array of processing locales. English locale is always present for category matching.
  if (m_currentLocaleCode != -1)
    result.push_back(m_currentLocaleCode);
  if (m_inputLocaleCode != -1 && m_inputLocaleCode != m_currentLocaleCode)
    result.push_back(m_inputLocaleCode);
  if (enLocaleCode != m_currentLocaleCode && enLocaleCode != m_inputLocaleCode)
    result.push_back(enLocaleCode);

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

void Processor::Search(SearchParams const & params, m2::RectD const & viewport)
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
  SetViewport(viewport, true /* forceUpdate */);
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
  m_emitter.AddResultNoChecks(m_ranker.MakeResult(PreResult2(lat, lon)));
  m_emitter.Emit();
}

namespace
{
int GetOldTypeFromIndex(size_t index)
{
  // "building" has old type value = 70
  ASSERT_NOT_EQUAL(index, 70, ());

  switch (index)
  {
  case 156: return 4099;
  case 98: return 4163;
  case 374: return 4419;
  case 188: return 4227;
  case 100: return 6147;
  case 107: return 4547;
  case 96: return 5059;
  case 60: return 6275;
  case 66: return 5251;
  case 161: return 4120;
  case 160: return 4376;
  case 159: return 4568;
  case 16: return 4233;
  case 178: return 5654;
  case 227: return 4483;
  case 111: return 5398;
  case 256: return 5526;
  case 702: return 263446;
  case 146: return 4186;
  case 155: return 4890;
  case 141: return 4570;
  case 158: return 4762;
  case 38: return 5891;
  case 63: return 4291;
  case 270: return 4355;
  case 327: return 4675;
  case 704: return 4611;
  case 242: return 4739;
  case 223: return 4803;
  case 174: return 4931;
  case 137: return 5123;
  case 186: return 5187;
  case 250: return 5315;
  case 104: return 4299;
  case 113: return 5379;
  case 206: return 4867;
  case 184: return 5443;
  case 125: return 5507;
  case 170: return 5571;
  case 25: return 5763;
  case 118: return 5827;
  case 76: return 6019;
  case 116: return 6083;
  case 108: return 6211;
  case 35: return 6339;
  case 180: return 6403;
  case 121: return 6595;
  case 243: return 6659;
  case 150: return 6723;
  case 175: return 6851;
  case 600: return 4180;
  case 348: return 4244;
  case 179: return 4116;
  case 77: return 4884;
  case 387: return 262164;
  case 214: return 4308;
  case 289: return 4756;
  case 264: return 4692;
  case 93: return 4500;
  case 240: return 4564;
  case 127: return 4820;
  case 29: return 4436;
  case 20: return 4948;
  case 18: return 4628;
  case 293: return 4372;
  case 22: return 4571;
  case 3: return 4699;
  case 51: return 4635;
  case 89: return 4123;
  case 307: return 5705;
  case 15: return 5321;
  case 6: return 4809;
  case 58: return 6089;
  case 26: return 5513;
  case 187: return 5577;
  case 1: return 5769;
  case 12: return 5897;
  case 244: return 5961;
  case 8: return 6153;
  case 318: return 6217;
  case 2: return 6025;
  case 30: return 5833;
  case 7: return 6281;
  case 65: return 6409;
  case 221: return 6473;
  case 54: return 4937;
  case 69: return 5385;
  case 4: return 6537;
  case 200: return 5257;
  case 195: return 5129;
  case 120: return 5193;
  case 56: return 5904;
  case 5: return 6864;
  case 169: return 4171;
  case 61: return 5707;
  case 575: return 5968;
  case 563: return 5456;
  case 13: return 6992;
  case 10: return 4811;
  case 109: return 4236;
  case 67: return 4556;
  case 276: return 4442;
  case 103: return 4506;
  case 183: return 4440;
  case 632: return 4162;
  case 135: return 4098;
  case 205: return 5004;
  case 87: return 4684;
  case 164: return 4940;
  case 201: return 4300;
  case 68: return 4620;
  case 101: return 5068;
  case 0: return 70;
  case 737: return 4102;
  case 703: return 5955;
  case 705: return 6531;
  case 706: return 5635;
  case 707: return 5699;
  case 708: return 4995;
  case 715: return 4298;
  case 717: return 4362;
  case 716: return 4490;
  case 718: return 4234;
  case 719: return 4106;
  case 722: return 4240;
  case 723: return 6480;
  case 725: return 4312;
  case 726: return 4248;
  case 727: return 4184;
  case 728: return 4504;
  case 732: return 4698;
  case 733: return 4378;
  case 734: return 4634;
  case 166: return 4250;
  case 288: return 4314;
  case 274: return 4122;
  }
  return -1;
}
}  // namespace

void Processor::InitParams(QueryParams & params)
{
  if (m_prefix.empty())
    params.InitNoPrefix(m_tokens.begin(), m_tokens.end());
  else
    params.InitWithPrefix(m_tokens.begin(), m_tokens.end(), m_prefix);

  // Add names of categories (and synonyms).
  Classificator const & c = classif();
  auto addSyms = [&](size_t i, uint32_t t)
  {
    uint32_t const index = c.GetIndexForType(t);
    params.GetTypeIndices(i).push_back(index);

    // v2-version MWM has raw classificator types in search index prefix, so
    // do the hack: add synonyms for old convention if needed.
    if (m_supportOldFormat)
    {
      int const type = GetOldTypeFromIndex(index);
      if (type >= 0)
      {
        ASSERT(type == 70 || type > 4000, (type));
        params.GetTypeIndices(i).push_back(static_cast<uint32_t>(type));
      }
    }
  };

  // todo(@m, @y). Shall we match prefix tokens for categories?
  ForEachCategoryTypeFuzzy(QuerySliceOnRawStrings<decltype(m_tokens)>(m_tokens, m_prefix), addSyms);

  RemoveStopWordsIfNeeded(params);

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
    params.m_pivot = m_viewport[CURRENT_V];
  else
    params.m_pivot = GetPivotRect();
  params.m_hotelsFilter = m_hotelsFilter;
  params.m_cianMode = m_cianMode;
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
  for (size_t i = 0; i < COUNT_V; ++i)
    ClearCache(i);

  m_geocoder.ClearCaches();
  m_villagesCache.Clear();
  m_preRanker.ClearCaches();
  m_ranker.ClearCaches();
}

m2::RectD const & Processor::GetViewport(ViewportID vID /*= DEFAULT_V*/) const
{
  if (vID == LOCALITY_V)
  {
    // special case for search address - return viewport around location
    return m_viewport[vID];
  }

  ASSERT(m_viewport[CURRENT_V].IsValid(), ());
  return m_viewport[CURRENT_V];
}

string DebugPrint(Processor::ViewportID viewportId)
{
  switch (viewportId)
  {
  case Processor::DEFAULT_V: return "Default";
  case Processor::CURRENT_V: return "Current";
  case Processor::LOCALITY_V: return "Locality";
  case Processor::COUNT_V: return "Count";
  }
  ASSERT(false, ("Unknown viewportId"));
  return "Unknown";
}
}  // namespace search
