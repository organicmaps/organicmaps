#include "search/processor.hpp"

#include "ge0/parser.hpp"

#include "search/common.hpp"
#include "search/cuisine_filter.hpp"
#include "search/dummy_rank_table.hpp"
#include "search/geometry_utils.hpp"
#include "search/intermediate_result.hpp"
#include "search/latlon_match.hpp"
#include "search/mode.hpp"
#include "search/postcode_points.hpp"
#include "search/pre_ranking_info.hpp"
#include "search/query_params.hpp"
#include "search/ranking_info.hpp"
#include "search/ranking_utils.hpp"
#include "search/search_index_values.hpp"
#include "search/search_params.hpp"
#include "search/utils.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage_defines.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"
#include "indexer/data_source.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_covering.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/postcodes_matcher.hpp"
#include "indexer/scales.hpp"
#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"
#include "indexer/trie_reader.hpp"

#include "platform/mwm_traits.hpp"
#include "platform/mwm_version.hpp"
#include "platform/preferred_languages.hpp"

#include "coding/compressed_bit_vector.hpp"
#include "coding/reader_wrapper.hpp"
#include "coding/string_utf8_multilang.hpp"
#include "coding/url.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/buffer_vector.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cctype>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <utility>

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
  LANGUAGE_TIER_ALT_AND_OLD,
  LANGUAGE_TIER_COUNT
};

m2::RectD GetRectAroundPosition(m2::PointD const & position)
{
  double constexpr kMaxPositionRadiusM = 50.0 * 1000;
  return mercator::RectByCenterXYAndSizeInMeters(position, kMaxPositionRadiusM);
}

string GetStatisticsMode(SearchParams const & params)
{
  switch (params.m_mode)
  {
  case Mode::Viewport:
    return "Viewport";
  case Mode::Everywhere:
    return "Everywhere";
  case Mode::Downloader:
    return "Downloader";
  case Mode::Bookmarks:
    if (params.m_bookmarksGroupId != bookmarks::kInvalidGroupId)
      return "BookmarksList";
    return "Bookmarks";
  case Mode::Count:
    UNREACHABLE();
  }
  UNREACHABLE();
}

void SendStatistics(SearchParams const & params, m2::RectD const & viewport, Results const & res)
{

  string resultString = strings::to_string(res.GetCount());
  for (size_t i = 0; i < res.GetCount(); ++i)
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
      {"mode", GetStatisticsMode(params)}
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

void TrimLeadingSpaces(string & s)
{
  while (!s.empty() && isspace(s.front()))
    s = s.substr(1);
}

bool EatFid(string & s, uint32_t & fid)
{
  TrimLeadingSpaces(s);

  if (s.empty())
    return false;

  size_t i = 0;
  while (i < s.size() && isdigit(s[i]))
    ++i;

  auto const prefix = s.substr(0, i);
  if (strings::to_uint32(prefix, fid))
  {
    s = s.substr(prefix.size());
    return true;
  }
  return false;
}

bool EatMwmName(base::MemTrie<storage::CountryId, base::VectorValues<bool>> const & countriesTrie,
                string & s, storage::CountryId & mwmName)
{
  TrimLeadingSpaces(s);

  // Greedily eat as much as possible because some country names are prefixes of others.
  optional<size_t> lastPos;
  for (size_t i = 0; i < s.size(); ++i)
  {
    // todo(@m) This must be much faster but MemTrie's iterators do not expose nodes.
    if (countriesTrie.HasKey(s.substr(0, i)))
      lastPos = i;
  }
  if (!lastPos)
    return false;

  mwmName = s.substr(0, *lastPos);
  s = s.substr(*lastPos);
  strings::EatPrefix(s, ".mwm");
  return true;
}

bool EatVersion(string & s, uint32_t & version)
{
  TrimLeadingSpaces(s);

  if (!s.empty() && s.front() == '0' && (s.size() == 1 || !isdigit(s[1])))
  {
    version = 0;
    s = s.substr(1);
    return true;
  }

  size_t const kVersionLength = 6;
  if (s.size() >= kVersionLength && all_of(s.begin(), s.begin() + kVersionLength, ::isdigit) &&
      (s.size() == kVersionLength || !isdigit(s[kVersionLength + 1])))
  {
    VERIFY(strings::to_uint32(s.substr(0, kVersionLength), version), ());
    s = s.substr(kVersionLength);
    return true;
  }

  return false;
}
}  // namespace

Processor::Processor(DataSource const & dataSource, CategoriesHolder const & categories,
                     vector<Suggest> const & suggests,
                     storage::CountryInfoGetter const & infoGetter)
  : m_categories(categories)
  , m_infoGetter(infoGetter)
  , m_dataSource(dataSource)
  , m_localitiesCaches(static_cast<base::Cancellable const &>(*this))
  , m_citiesBoundaries(m_dataSource)
  , m_keywordsScorer(LanguageTier::LANGUAGE_TIER_COUNT)
  , m_ranker(m_dataSource, m_citiesBoundaries, infoGetter, m_keywordsScorer, m_emitter, categories,
             suggests, m_localitiesCaches.m_villages, static_cast<base::Cancellable const &>(*this))
  , m_preRanker(m_dataSource, m_ranker)
  , m_geocoder(m_dataSource, infoGetter, categories, m_citiesBoundaries, m_preRanker,
               m_localitiesCaches, static_cast<base::Cancellable const &>(*this))
  , m_bookmarksProcessor(m_emitter, static_cast<base::Cancellable const &>(*this))
{
  // Current and input langs are to be set later.
  m_keywordsScorer.SetLanguages(
      LanguageTier::LANGUAGE_TIER_EN_AND_INTERNATIONAL,
      {StringUtf8Multilang::kInternationalCode, StringUtf8Multilang::kEnglishCode});
  m_keywordsScorer.SetLanguages(LanguageTier::LANGUAGE_TIER_DEFAULT,
                                {StringUtf8Multilang::kDefaultCode});
  m_keywordsScorer.SetLanguages(
      LanguageTier::LANGUAGE_TIER_ALT_AND_OLD,
      {StringUtf8Multilang::kAltNameCode, StringUtf8Multilang::kOldNameCode});

  SetPreferredLocale("en");

  for (auto const & country : m_infoGetter.GetCountries())
    m_countriesTrie.Add(country.m_countryId, true);
}

void Processor::SetViewport(m2::RectD const & viewport)
{
  ASSERT(viewport.IsValid(), ());

  if (m_viewport.IsValid())
  {
    double constexpr kEpsMeters = 10.0;
    double const kEps = mercator::MetersToMercator(kEpsMeters);
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
  m_keywordsScorer.SetLanguages(LanguageTier::LANGUAGE_TIER_CURRENT, feature::GetSimilar(code));

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

  int8_t const code = StringUtf8Multilang::GetLangIndex(languages::Normalize(locale));
  LOG(LDEBUG, ("New input locale:", locale, "locale code:", code));
  m_keywordsScorer.SetLanguages(LanguageTier::LANGUAGE_TIER_INPUT, feature::GetSimilar(code));
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
    auto normalizedQuery = NormalizeAndSimplifyString(query);
    PreprocessBeforeTokenization(normalizedQuery);
    SplitUniString(normalizedQuery, base::MakeBackInsertFunctor(tokens), delims);
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

  if (m_isCategorialRequest)
    m_emitter.PrecheckHotelQuery(m_preferredTypes);

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

void Processor::CacheWorldLocalities() { m_geocoder.CacheWorldLocalities(); }

void Processor::LoadCitiesBoundaries()
{
  if (m_citiesBoundaries.Load())
    LOG(LINFO, ("Loaded cities boundaries"));
  else
    LOG(LWARNING, ("Can't load cities boundaries"));
}

void Processor::LoadCountriesTree() { m_ranker.LoadCountriesTree(); }

void Processor::EnableIndexingOfBookmarksDescriptions(bool enable)
{
  m_bookmarksProcessor.EnableIndexingOfDescriptions(enable);
}

void Processor::EnableIndexingOfBookmarkGroup(bookmarks::GroupId const & groupId, bool enable)
{
  m_bookmarksProcessor.EnableIndexingOfBookmarkGroup(groupId, enable);
}

void Processor::ResetBookmarks()
{
  m_bookmarksProcessor.Reset();
}

void Processor::OnBookmarksCreated(vector<pair<bookmarks::Id, bookmarks::Doc>> const & marks)
{
  for (auto const & idDoc : marks)
    m_bookmarksProcessor.Add(idDoc.first /* id */, idDoc.second /* doc */);
}

void Processor::OnBookmarksUpdated(vector<pair<bookmarks::Id, bookmarks::Doc>> const & marks)
{
  for (auto const & idDoc : marks)
    m_bookmarksProcessor.Update(idDoc.first /* id */, idDoc.second /* doc */);
}

void Processor::OnBookmarksDeleted(vector<bookmarks::Id> const & marks)
{
  for (auto const & id : marks)
    m_bookmarksProcessor.Erase(id);
}

void Processor::OnBookmarksAttachedToGroup(bookmarks::GroupId const & groupId,
                                           vector<bookmarks::Id> const & marks)
{
  for (auto const & id : marks)
    m_bookmarksProcessor.AttachToGroup(id, groupId);
}

void Processor::OnBookmarksDetachedFromGroup(bookmarks::GroupId const & groupId,
                                             vector<bookmarks::Id> const & marks)
{
  for (auto const & id : marks)
    m_bookmarksProcessor.DetachFromGroup(id, groupId);
}

void Processor::Reset()
{
  base::Cancellable::Reset();
  m_lastUpdate = false;
}

bool Processor::IsCancelled() const
{
  if (m_lastUpdate)
    return false;

  bool const ret = base::Cancellable::IsCancelled();
  bool const byDeadline = CancellationStatus() == base::Cancellable::Status::DeadlineExceeded;

  // todo(@m) This is a "soft deadline": we ignore it if nothing has been
  //          found so far. We could also implement a "hard deadline"
  //          that would be impossible to ignore.
  if (byDeadline && m_preRanker.Size() == 0 && m_preRanker.NumSentResults() == 0)
    return false;

  return ret;
}

void Processor::SearchByFeatureId()
{
  // String processing is suboptimal in this method so
  // we need a guard against very long strings.
  size_t const kMaxFeatureIdStringSize = 1000;
  if (m_query.size() > kMaxFeatureIdStringSize)
    return;

  // Create a copy of the query to trim it in-place.
  string query(m_query);
  strings::Trim(query);

  strings::EatPrefix(query, "?");

  string const kFidPrefix = "fid";
  bool hasFidPrefix = false;

  if (strings::EatPrefix(query, kFidPrefix))
  {
    hasFidPrefix = true;

    strings::Trim(query);
    if (strings::EatPrefix(query, "="))
      strings::Trim(query);
  }

  vector<shared_ptr<MwmInfo>> infos;
  m_dataSource.GetMwmsInfo(infos);

  // Case 0.
  if (hasFidPrefix)
  {
    string s = query;
    uint32_t fid;
    if (EatFid(s, fid))
      EmitFeaturesByIndexFromAllMwms(infos, fid);
  }

  // Case 1.
  if (hasFidPrefix)
  {
    string s = query;
    storage::CountryId mwmName;
    uint32_t fid;

    bool ok = true;
    bool const parenPref = strings::EatPrefix(s, "(");
    bool const parenSuff = strings::EatSuffix(s, ")");
    ok = ok && parenPref == parenSuff;
    ok = ok && EatMwmName(m_countriesTrie, s, mwmName);
    ok = ok && strings::EatPrefix(s, ",");
    ok = ok && EatFid(s, fid);
    if (ok)
      EmitFeatureIfExists(infos, mwmName, {} /* version */, fid);
  }

  // Case 2.
  {
    string s = query;
    storage::CountryId mwmName;
    uint32_t version;
    uint32_t fid;

    bool ok = true;
    ok = ok && strings::EatPrefix(s, "{ MwmId [");
    ok = ok && EatMwmName(m_countriesTrie, s, mwmName);
    ok = ok && strings::EatPrefix(s, ", ");
    ok = ok && EatVersion(s, version);
    ok = ok && strings::EatPrefix(s, "], ");
    ok = ok && EatFid(s, fid);
    ok = ok && strings::EatPrefix(s, " }");
    if (ok)
      EmitFeatureIfExists(infos, mwmName, version, fid);
  }
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
  SetDeadline(chrono::steady_clock::now() + params.m_timeout);

  InitEmitter(params);

  if (params.m_onStarted)
    params.m_onStarted();

  // IsCancelled is not enough here because it depends on PreRanker being initialized.
  if (IsCancelled() && CancellationStatus() == base::Cancellable::Status::CancelCalled)
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

  // Used to store the earliest available cancellation status:
  // if the search has been cancelled, we need to pinpoint the reason
  // for cancellation and a further call to CancellationStatus() may
  // return a different result.
  auto cancellationStatus = base::Cancellable::Status::Active;

  switch (params.m_mode)
  {
  case Mode::Everywhere:  // fallthrough
  case Mode::Viewport:    // fallthrough
  case Mode::Downloader:
  {
    Geocoder::Params geocoderParams;
    InitGeocoder(geocoderParams, params);
    InitPreRanker(geocoderParams, params);
    InitRanker(geocoderParams, params);

    try
    {
      SearchDebug();
      SearchCoordinates();
      SearchPlusCode();
      SearchPostcode();
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
    }
    catch (CancelException const &)
    {
      LOG(LDEBUG, ("Search has been cancelled. Reason:", CancellationStatus()));
    }

    cancellationStatus = CancellationStatus();
    if (cancellationStatus != base::Cancellable::Status::CancelCalled)
    {
      m_lastUpdate = true;
      // Cancellable is effectively disabled now, so
      // this call must not result in a CancelException.
      m_preRanker.UpdateResults(true /* lastUpdate */);
    }

    // Emit finish marker to client.
    m_geocoder.Finish(cancellationStatus == Cancellable::Status::CancelCalled);
    break;
  }
  case Mode::Bookmarks: SearchBookmarks(params.m_bookmarksGroupId); break;
  case Mode::Count: ASSERT(false, ("Invalid mode")); break;
  }

  // todo(@m) Send the fact of cancelling by timeout to stats?
  if (!viewportSearch && cancellationStatus != Cancellable::Status::CancelCalled)
    SendStatistics(params, viewport, m_emitter.GetResults());
}

void Processor::SearchDebug()
{
  SearchByFeatureId();
}

void Processor::SearchCoordinates()
{
  buffer_vector<ms::LatLon, 3> results;

  {
    double lat;
    double lon;
    if (MatchLatLonDegree(m_query, lat, lon))
      results.emplace_back(lat, lon);
  }

  istringstream iss(m_query);
  string token;
  while (iss >> token)
  {
    ge0::Ge0Parser parser;
    ge0::Ge0Parser::Result r;
    if (parser.Parse(token, r))
      results.emplace_back(r.m_lat, r.m_lon);

    url::GeoURLInfo info(token);
    if (info.IsValid())
      results.emplace_back(info.m_lat, info.m_lon);
  }

  base::SortUnique(results);
  for (auto const & r : results)
  {
    m_emitter.AddResultNoChecks(m_ranker.MakeResult(
        RankerResult(r.m_lat, r.m_lon), true /* needAddress */, true /* needHighlighting */));
    m_emitter.Emit();
  }
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
    ms::LatLon const latLon = mercator::ToLatLon(*m_position);
    code = openlocationcode::RecoverNearest(query, {latLon.m_lat, latLon.m_lon});
  }

  if (code.empty())
    return;

  openlocationcode::CodeArea const area = openlocationcode::Decode(code);
  m_emitter.AddResultNoChecks(
      m_ranker.MakeResult(RankerResult(area.GetCenter().latitude, area.GetCenter().longitude),
                          true /* needAddress */, false /* needHighlighting */));
  m_emitter.Emit();
}

void Processor::SearchPostcode()
{
  // Create a copy of the query to trim it in-place.
  string query(m_query);
  strings::Trim(query);

  if (!LooksLikePostcode(query, !m_prefix.empty()))
    return;

  vector<shared_ptr<MwmInfo>> infos;
  m_dataSource.GetMwmsInfo(infos);

  for (auto const & info : infos)
  {
    auto handle = m_dataSource.GetMwmHandleById(MwmSet::MwmId(info));
    if (!handle.IsAlive())
      continue;
    auto & value = *handle.GetValue();
    if (!value.m_cont.IsExist(POSTCODE_POINTS_FILE_TAG))
      continue;

    PostcodePoints postcodes(value);

    vector<m2::PointD> points;
    postcodes.Get(NormalizeAndSimplifyString(query), points);
    if (points.empty())
      continue;

    m2::RectD r;
    for (auto const & p : points)
      r.Add(p);

    m_emitter.AddResultNoChecks(m_ranker.MakeResult(
        RankerResult(r.Center(), query), true /* needAddress */, false /* needHighlighting */));
    m_emitter.Emit();
    return;
  }
}

void Processor::SearchBookmarks(bookmarks::GroupId const & groupId)
{
  bookmarks::Processor::Params params;
  InitParams(params);
  params.m_groupId = groupId;

  try
  {
    m_bookmarksProcessor.Search(params);
  }
  catch (CancelException const &)
  {
    LOG(LDEBUG, ("Bookmarks search has been cancelled."));
  }

  // Emit finish marker to client.
  m_bookmarksProcessor.Finish(IsCancelled());
}

void Processor::InitParams(QueryParams & params) const
{
  params.SetQuery(m_query);

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
    if (IsStreetSynonym(token.GetOriginal()))
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
  geocoderParams.m_streetSearchRadiusM = searchParams.m_streetSearchRadiusM;
  geocoderParams.m_villageSearchRadiusM = searchParams.m_villageSearchRadiusM;

  m_geocoder.SetParams(geocoderParams);
}

void Processor::InitPreRanker(Geocoder::Params const & geocoderParams,
                              SearchParams const & searchParams)
{
  bool const viewportSearch = searchParams.m_mode == Mode::Viewport;

  PreRanker::Params params;

  if (viewportSearch)
  {
    params.m_minDistanceOnMapBetweenResultsX = searchParams.m_minDistanceOnMapBetweenResultsX;
    params.m_minDistanceOnMapBetweenResultsY = searchParams.m_minDistanceOnMapBetweenResultsY;
  }

  params.m_viewport = GetViewport();
  params.m_accuratePivotCenter = GetPivotPoint(viewportSearch);
  params.m_position = m_position;
  params.m_scale = geocoderParams.m_scale;
  params.m_limit = max(SearchParams::kPreResultsCount, searchParams.m_maxNumResults);
  params.m_viewportSearch = viewportSearch;
  params.m_categorialRequest = geocoderParams.IsCategorialRequest();
  params.m_numQueryTokens = geocoderParams.GetNumTokens();

  m_preRanker.Init(params);
}

void Processor::InitRanker(Geocoder::Params const & geocoderParams,
                           SearchParams const & searchParams)
{
  bool const viewportSearch = searchParams.m_mode == Mode::Viewport;

  Ranker::Params params;

  params.m_currentLocaleCode = m_currentLocaleCode;

  params.m_batchSize = searchParams.m_batchSize;
  params.m_limit = searchParams.m_maxNumResults;
  params.m_pivot = m_position ? *m_position : GetViewport().Center();
  params.m_pivotRegion = GetPivotRegion();
  params.m_preferredTypes = m_preferredTypes;
  params.m_suggestsEnabled = searchParams.m_suggestsEnabled;
  params.m_needAddress = searchParams.m_needAddress;
  params.m_needHighlighting = searchParams.m_needHighlighting && !geocoderParams.IsCategorialRequest();
  params.m_query = m_query;
  params.m_tokens = m_tokens;
  params.m_prefix = m_prefix;
  params.m_categoryLocales = GetCategoryLocales();
  params.m_accuratePivotCenter = GetPivotPoint(viewportSearch);
  params.m_viewportSearch = viewportSearch;
  params.m_viewport = GetViewport();
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
  m_localitiesCaches.Clear();
  m_preRanker.ClearCaches();
  m_ranker.ClearCaches();
  m_viewport.MakeEmpty();
}

void Processor::EmitFeatureIfExists(vector<shared_ptr<MwmInfo>> const & infos,
                                    storage::CountryId const & mwmName, optional<uint32_t> version,
                                    uint32_t fid)
{
  for (auto const & info : infos)
  {
    if (info->GetCountryName() != mwmName)
      continue;

    if (version && version != info->GetVersion())
      continue;

    auto guard = make_unique<FeaturesLoaderGuard>(m_dataSource, MwmSet::MwmId(info));
    if (fid >= guard->GetNumFeatures())
      continue;
    auto ft = guard->GetFeatureByIndex(fid);
    if (!ft)
      continue;
    auto const center = feature::GetCenter(*ft, FeatureType::WORST_GEOMETRY);
    string name;
    ft->GetReadableName(name);
    m_emitter.AddResultNoChecks(m_ranker.MakeResult(
        RankerResult(*ft, center, m2::PointD() /* pivot */, name, guard->GetCountryFileName()),
        true /* needAddress */, true /* needHighlighting */));
    m_emitter.Emit();
  }
}

void Processor::EmitFeaturesByIndexFromAllMwms(vector<shared_ptr<MwmInfo>> const & infos,
                                               uint32_t fid)
{
  vector<tuple<double, m2::PointD, std::string, std::unique_ptr<FeatureType>>> results;
  vector<unique_ptr<FeaturesLoaderGuard>> guards;
  for (auto const & info : infos)
  {
    auto guard = make_unique<FeaturesLoaderGuard>(m_dataSource, MwmSet::MwmId(info));
    if (fid >= guard->GetNumFeatures())
      continue;
    auto ft = guard->GetFeatureByIndex(fid);
    if (!ft)
      continue;
    auto const center = feature::GetCenter(*ft, FeatureType::WORST_GEOMETRY);
    double dist = center.SquaredLength(m_viewport.Center());
    auto pivot = m_viewport.Center();
    if (m_position)
    {
      auto const distPos = center.SquaredLength(*m_position);
      if (dist > distPos)
      {
        dist = distPos;
        pivot = *m_position;
      }
    }
    results.emplace_back(dist, pivot, guard->GetCountryFileName(), move(ft));
    guards.push_back(move(guard));
  }
  sort(results.begin(), results.end());
  for (auto const & [dist, pivot, country, ft] : results)
  {
    auto const center = feature::GetCenter(*ft, FeatureType::WORST_GEOMETRY);
    string name;
    ft->GetReadableName(name);
    m_emitter.AddResultNoChecks(m_ranker.MakeResult(RankerResult(*ft, center, pivot, name, country),
                                                    true /* needAddress */,
                                                    true /* needHighlighting */));
    m_emitter.Emit();
  }
}
}  // namespace search
