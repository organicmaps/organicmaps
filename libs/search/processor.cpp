#include "search/processor.hpp"

#include "ge0/parser.hpp"

#include "search/common.hpp"
#include "search/geometry_utils.hpp"
#include "search/intermediate_result.hpp"
#include "search/latlon_match.hpp"
#include "search/mode.hpp"
#include "search/postcode_points.hpp"
#include "search/query_params.hpp"
#include "search/ranking_utils.hpp"
#include "search/search_params.hpp"
#include "search/utils.hpp"
#include "search/utm_mgrs_coords_match.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage_defines.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"
#include "indexer/data_source.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/postcodes_matcher.hpp"
#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "platform/preferred_languages.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/buffer_vector.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <sstream>

#include "3party/open-location-code/openlocationcode.h"

namespace search
{
using namespace std;

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

// Removes all full-token stop words from |tokens|.
// Does nothing if all tokens are non-prefix stop words.
void RemoveStopWordsIfNeeded(QueryTokens & tokens, strings::UniString & prefix)
{
  size_t numStopWords = 0;
  for (auto const & token : tokens)
    if (IsStopWord(token))
      ++numStopWords;

  if (numStopWords == tokens.size() && prefix.empty())
    return;

  tokens.erase_if(&IsStopWord);
}

void TrimLeadingSpaces(string & s)
{
  while (!s.empty() && strings::IsASCIISpace(s.front()))
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

bool EatMwmName(base::MemTrie<storage::CountryId, base::VectorValues<bool>> const & countriesTrie, string & s,
                storage::CountryId & mwmName)
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
                     vector<Suggest> const & suggests, storage::CountryInfoGetter const & infoGetter)
  : m_categories(categories)
  , m_infoGetter(infoGetter)
  , m_dataSource(dataSource)
  , m_localitiesCaches(static_cast<base::Cancellable const &>(*this))
  , m_citiesBoundaries(m_dataSource)
  , m_keywordsScorer(LanguageTier::LANGUAGE_TIER_COUNT)
  , m_ranker(m_dataSource, m_citiesBoundaries, infoGetter, m_keywordsScorer, m_emitter, categories, suggests,
             m_localitiesCaches.m_villages, static_cast<base::Cancellable const &>(*this))
  , m_preRanker(m_dataSource, m_ranker)
  , m_geocoder(m_dataSource, infoGetter, categories, m_citiesBoundaries, m_preRanker, m_localitiesCaches,
               static_cast<base::Cancellable const &>(*this))
  , m_bookmarksProcessor(m_emitter, static_cast<base::Cancellable const &>(*this))
{
  // Current and input langs are to be set later.
  m_keywordsScorer.SetLanguages(LanguageTier::LANGUAGE_TIER_EN_AND_INTERNATIONAL,
                                {StringUtf8Multilang::kInternationalCode, StringUtf8Multilang::kEnglishCode});
  m_keywordsScorer.SetLanguages(LanguageTier::LANGUAGE_TIER_DEFAULT, {StringUtf8Multilang::kDefaultCode});
  m_keywordsScorer.SetLanguages(LanguageTier::LANGUAGE_TIER_ALT_AND_OLD,
                                {StringUtf8Multilang::kAltNameCode, StringUtf8Multilang::kOldNameCode});

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
  LOG(LDEBUG, ("New input locale:", locale, "; locale code:", int(code)));
  m_keywordsScorer.SetLanguages(LanguageTier::LANGUAGE_TIER_INPUT, feature::GetSimilar(code));
  m_inputLocaleCode = CategoriesHolder::MapLocaleToInteger(locale);
}

void Processor::SetQuery(string const & query, bool categorialRequest /* = false */)
{
  LOG(LDEBUG, ("query:", query, "isCategorial:", categorialRequest));

  m_query.m_query = query;
  m_query.m_tokens.clear();
  m_query.m_prefix.clear();

  // Following code splits input query by delimiters except hash tags
  // first, and then splits result tokens by hashtags. The goal is to
  // retrieve all tokens that start with a single hashtag and leave
  // them as is.

  Delimiters delims;
  auto normalizedQuery = NormalizeAndSimplifyString(query);
  PreprocessBeforeTokenization(normalizedQuery);
  SplitUniString(normalizedQuery, base::MakeBackInsertFunctor(m_query.m_tokens), delims);

  static_assert(kMaxNumTokens > 0, "");
  size_t const maxTokensCount = kMaxNumTokens - 1;
  if (m_query.m_tokens.size() > maxTokensCount)
  {
    m_query.m_tokens.resize(maxTokensCount);
  }
  else
  {
    // Assign the last parsed token to prefix.
    if (!m_query.m_tokens.empty() && !delims(normalizedQuery.back()))
    {
      m_query.m_prefix.swap(m_query.m_tokens.back());
      m_query.m_tokens.pop_back();
    }
  }

  QuerySliceOnRawStrings const tokenSlice(m_query.m_tokens, m_query.m_prefix);

  // Get preferred types to show in results.
  m_preferredTypes.clear();
  m_isCategorialRequest = categorialRequest;

  auto const locales = GetCategoryLocales();
  if (!FillCategories(tokenSlice, locales, m_categories, m_preferredTypes))
  {
    // Try to match query to cuisine categories.
    if (FillCategories(tokenSlice, locales, GetDefaultCuisineCategories(), m_cuisineTypes))
    {
      /// @todo What if I'd like to find "Burger" street? @see "BurgerStreet" test.
      m_isCategorialRequest = true;
      m_preferredTypes = ftypes::IsEatChecker::Instance().GetTypes();
    }
  }

  // Remove stopwords *after* FillCategories call (it makes exact tokens match).
  RemoveStopWordsIfNeeded(m_query.m_tokens, m_query.m_prefix);

  if (!m_isCategorialRequest)
  {
    // Assign tokens and prefix to scorer.
    m_keywordsScorer.SetKeywords(m_query);

    ForEachCategoryType(tokenSlice, [&](size_t, uint32_t t) { m_preferredTypes.push_back(t); });
  }

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

void Processor::CacheWorldLocalities()
{
  m_geocoder.CacheWorldLocalities();
}

void Processor::LoadCitiesBoundaries()
{
  if (m_citiesBoundaries.Load())
    LOG(LINFO, ("Loaded cities boundaries"));
  else
    LOG(LWARNING, ("Can't load cities boundaries"));
}

void Processor::LoadCountriesTree()
{
  m_ranker.LoadCountriesTree();
}

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

void Processor::OnBookmarksAttachedToGroup(bookmarks::GroupId const & groupId, vector<bookmarks::Id> const & marks)
{
  for (auto const & id : marks)
    m_bookmarksProcessor.AttachToGroup(id, groupId);
}

void Processor::OnBookmarksDetachedFromGroup(bookmarks::GroupId const & groupId, vector<bookmarks::Id> const & marks)
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
  // Create a copy of the query to trim it in-place.
  string query(m_query.m_query);
  strings::Trim(query);

  if (strings::EatPrefix(query, "?fid"))
  {
    strings::Trim(query);
    if (strings::EatPrefix(query, "="))
      strings::Trim(query);
  }
  else
    return;

  vector<shared_ptr<MwmInfo>> infos;
  m_dataSource.GetMwmsInfo(infos);

  auto const putFeature = [](FeaturesLoaderGuard & guard, uint32_t fid, auto const & fn)
  {
    // Feature can be deleted, so always check.
    auto ft = guard.GetFeatureByIndex(fid);
    if (ft)
      fn(std::move(ft));
  };

  storage::CountryId mwmName;
  uint32_t version;
  uint32_t fid;

  // Case 0.
  {
    string s = query;
    if (EatFid(s, fid))
    {
      EmitResultsFromMwms(infos, [&putFeature, fid](FeaturesLoaderGuard & guard, auto const & fn)
      {
        if (fid < guard.GetNumFeatures())
          putFeature(guard, fid, fn);
      });
    }
  }

  // Case 1.
  {
    string s = query;
    bool const parenPref = strings::EatPrefix(s, "(");
    bool const parenSuff = strings::EatSuffix(s, ")");
    if (parenPref == parenSuff && EatMwmName(m_countriesTrie, s, mwmName) && strings::EatPrefix(s, ",") &&
        EatFid(s, fid))
    {
      EmitResultsFromMwms(infos, [&putFeature, &mwmName, fid](FeaturesLoaderGuard & guard, auto const & fn)
      {
        if (guard.GetCountryFileName() == mwmName && fid < guard.GetNumFeatures())
          putFeature(guard, fid, fn);
      });
    }
  }

  // Case 2.
  {
    string s = query;
    if (strings::EatPrefix(s, "{ MwmId [") && EatMwmName(m_countriesTrie, s, mwmName) && strings::EatPrefix(s, ", ") &&
        EatVersion(s, version) && strings::EatPrefix(s, "], ") && EatFid(s, fid) && strings::EatPrefix(s, " }"))
    {
      EmitResultsFromMwms(infos, [&putFeature, &mwmName, version, fid](FeaturesLoaderGuard & guard, auto const & fn)
      {
        if (guard.GetCountryFileName() == mwmName && guard.GetVersion() == version && fid < guard.GetNumFeatures())
          putFeature(guard, fid, fn);
      });
    }
  }
}

void Processor::EmitWithMetadata(feature::Metadata::EType type)
{
  std::vector<std::shared_ptr<MwmInfo>> infos;
  m_dataSource.GetMwmsInfo(infos);
  // less<MwmInfo*>
  std::sort(infos.begin(), infos.end());

  std::vector<FeatureID> ids;
  m_dataSource.ForEachFeatureIDInRect([&ids](FeatureID id) { ids.push_back(std::move(id)); }, m_viewport,
                                      scales::GetUpperScale());

  // The same, first criteria is less<MwmId> -> less<MwmInfo*>
  std::sort(ids.begin(), ids.end());

  size_t idx = 0;
  EmitResultsFromMwms(infos, [type, &idx, &ids](FeaturesLoaderGuard & guard, auto const & fn)
  {
    for (; idx < ids.size() && ids[idx].m_mwmId == guard.GetId(); ++idx)
    {
      auto ft = guard.GetFeatureByIndex(ids[idx].m_index);
      if (ft)
      {
        if (ft->HasMetadata(type))
          fn(std::move(ft));
      }
    }
  });
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
  ::search::ForEachCategoryType(slice, GetCategoryLocales(), m_categories, toDo);
}

template <typename ToDo>
void Processor::ForEachCategoryTypeFuzzy(StringSliceBase const & slice, ToDo && toDo) const
{
  ::search::ForEachCategoryTypeFuzzy(slice, GetCategoryLocales(), m_categories, toDo);
}

void Processor::Search(SearchParams params)
{
  /// @DebugNote
  // Comment this line to run search in a debugger.
  SetDeadline(chrono::steady_clock::now() + params.m_timeout);

  if (params.m_onStarted)
    params.m_onStarted();

  // IsCancelled is not enough here because it depends on PreRanker being initialized.
  if (IsCancelled() && CancellationStatus() == base::Cancellable::Status::CancelCalled)
  {
    Results results;
    results.SetEndMarker(true /* isCancelled */);
    params.m_onResults(std::move(results));
    return;
  }

  m_emitter.Init(std::move(params.m_onResults));

  bool const viewportSearch = params.m_mode == Mode::Viewport;

  auto const & viewport = params.m_viewport;
  ASSERT(viewport.IsValid(), ());

  m_position = params.m_position;

  SetInputLocale(params.m_inputLocale);

  SetQuery(params.m_query, params.m_categorialRequest);
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
      if (!SearchCoordinates() && !SearchDebug())
      {
        SearchPlusCode();
        SearchPostcode();
        if (viewportSearch)
        {
          m_geocoder.GoInViewport();
        }
        else
        {
          if (m_query.m_tokens.empty())
            m_ranker.SuggestStrings();
          m_geocoder.GoEverywhere();
        }
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

  if (!viewportSearch && cancellationStatus == Cancellable::Status::DeadlineExceeded)
    LOG(LWARNING, ("Search stopped by timeout"));
}

bool Processor::SearchDebug()
{
  if (m_query.m_query == "?wiki")
  {
    EmitWithMetadata(feature::Metadata::FMD_WIKIPEDIA);
    return true;
  }

  if (m_query.m_query == "?description")
  {
    EmitWithMetadata(feature::Metadata::FMD_DESCRIPTION);
    return true;
  }

#ifdef DEBUG
  SearchByFeatureId();

  if (m_query.m_query == "?euri")
  {
    EmitWithMetadata(feature::Metadata::FMD_EXTERNAL_URI);
    return true;
  }
#endif
  return false;
}

bool Processor::SearchCoordinates()
{
  bool coords_found = false;
  buffer_vector<ms::LatLon, 3> results;
  double lat, lon;

  if (MatchLatLonDegree(m_query.m_query, lat, lon))
  {
    coords_found = true;
    results.emplace_back(lat, lon);
  }

  auto ll = MatchUTMCoords(m_query.m_query);
  if (ll)
  {
    coords_found = true;
    results.emplace_back(ll->m_lat, ll->m_lon);
  }

  ll = MatchMGRSCoords(m_query.m_query);
  if (ll)
  {
    coords_found = true;
    results.emplace_back(ll->m_lat, ll->m_lon);
  }

  istringstream iss(m_query.m_query);
  string token;
  while (iss >> token)
  {
    ge0::Ge0Parser parser;
    ge0::Ge0Parser::Result r;
    if (parser.Parse(token, r))
    {
      coords_found = true;
      results.emplace_back(r.m_lat, r.m_lon);
    }

    geo::GeoURLInfo info;
    if (m_geoUrlParser.Parse(token, info))
    {
      coords_found = true;
      results.emplace_back(info.m_lat, info.m_lon);
    }
  }

  base::SortUnique(results);
  for (auto const & r : results)
  {
    m_emitter.AddResultNoChecks(
        m_ranker.MakeResult(RankerResult(r.m_lat, r.m_lon), true /* needAddress */, true /* needHighlighting */));
    m_emitter.Emit();
  }
  return coords_found;
}

void Processor::SearchPlusCode()
{
  // Create a copy of the query to trim it in-place.
  string query(m_query.m_query);
  strings::Trim(query);

  if (openlocationcode::IsFull(query))
  {
    openlocationcode::CodeArea const area = openlocationcode::Decode(query);
    m_emitter.AddResultNoChecks(m_ranker.MakeResult(RankerResult(area.GetCenter().latitude, area.GetCenter().longitude),
                                                    true /* needAddress */, false /* needHighlighting */));
  }
  else if (openlocationcode::IsShort(query))
  {
    string codeFromPos;

    if (m_position)
    {
      ms::LatLon const latLonFromPos = mercator::ToLatLon(*m_position);
      codeFromPos = openlocationcode::RecoverNearest(query, {latLonFromPos.m_lat, latLonFromPos.m_lon});
      openlocationcode::CodeArea const areaFromPos = openlocationcode::Decode(codeFromPos);

      m_emitter.AddResultNoChecks(
          m_ranker.MakeResult(RankerResult(areaFromPos.GetCenter().latitude, areaFromPos.GetCenter().longitude),
                              true /* needAddress */, false /* needHighlighting */));
    }

    ms::LatLon const latLonFromView = mercator::ToLatLon(m_viewport.Center());
    string codeFromView = openlocationcode::RecoverNearest(query, {latLonFromView.m_lat, latLonFromView.m_lon});

    if (codeFromView != codeFromPos)
    {
      openlocationcode::CodeArea const areaFromView = openlocationcode::Decode(codeFromView);

      m_emitter.AddResultNoChecks(
          m_ranker.MakeResult(RankerResult(areaFromView.GetCenter().latitude, areaFromView.GetCenter().longitude),
                              true /* needAddress */, false /* needHighlighting */));
    }
  }

  m_emitter.Emit();
}

void Processor::SearchPostcode()
{
  // Create a copy of the query to trim it in-place.
  string_view query(m_query.m_query);
  strings::Trim(query);

  /// @todo So, "G4 " now doesn't match UK (Glasgow) postcodes as prefix :)
  if (!LooksLikePostcode(query, !m_query.m_prefix.empty()))
    return;

  vector<shared_ptr<MwmInfo>> infos;
  m_dataSource.GetMwmsInfo(infos);

  auto const normQuery = NormalizeAndSimplifyString(query);
  for (auto const & info : infos)
  {
    auto handle = m_dataSource.GetMwmHandleById(MwmSet::MwmId(info));
    if (!handle.IsAlive())
      continue;
    auto & value = *handle.GetValue();
    if (!value.m_cont.IsExist(POSTCODE_POINTS_FILE_TAG))
      continue;

    /// @todo Well, ok for now, but we do only full or "prefix-rect" match w/o possible errors, with only one result.
    PostcodePoints postcodes(value);
    vector<m2::PointD> points;
    postcodes.Get(normQuery, points);
    if (points.empty())
      continue;

    m2::RectD r;
    for (auto const & p : points)
      r.Add(p);

    m_emitter.AddResultNoChecks(
        m_ranker.MakeResult(RankerResult(r.Center(), query), true /* needAddress */, true /* needHighlighting */));
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
  /// @todo Prettify
  params.Init(m_query.m_query, m_query.m_tokens.begin(), m_query.m_tokens.end(), m_query.m_prefix);

  Classificator const & c = classif();

  // Add names of categories (and synonyms).
  QuerySliceOnRawStrings const tokenSlice(m_query.m_tokens, m_query.m_prefix);
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
    ForEachCategoryTypeFuzzy(tokenSlice, [&c, &params](size_t i, uint32_t t)
    {
      uint32_t const index = c.GetIndexForType(t);
      params.GetTypeIndices(i).push_back(index);
    });
  }

  // Remove all type indices for streets, as they're considired individually.
  params.ClearStreetIndices();

  for (size_t i = 0; i < params.GetNumTokens(); ++i)
    base::SortUnique(params.GetTypeIndices(i));

  m_keywordsScorer.ForEachLanguage([&params](int8_t lang) { params.GetLangs().Insert(static_cast<uint64_t>(lang)); });
}

void Processor::InitGeocoder(Geocoder::Params & geocoderParams, SearchParams const & searchParams)
{
  auto const viewportSearch = searchParams.m_mode == Mode::Viewport;

  InitParams(geocoderParams);

  geocoderParams.m_mode = searchParams.m_mode;
  geocoderParams.m_pivot = GetPivotRect(viewportSearch);
  geocoderParams.m_position = m_position;
  geocoderParams.m_categoryLocales = GetCategoryLocales();
  geocoderParams.m_cuisineTypes = m_cuisineTypes;
  geocoderParams.m_preferredTypes = m_preferredTypes;
  geocoderParams.m_tracer = searchParams.m_tracer;
  geocoderParams.m_filteringParams = searchParams.m_filteringParams;
  geocoderParams.m_useDebugInfo = searchParams.m_useDebugInfo;

  m_geocoder.SetParams(geocoderParams);
}

void Processor::InitPreRanker(Geocoder::Params const & geocoderParams, SearchParams const & searchParams)
{
  bool const viewportSearch = searchParams.m_mode == Mode::Viewport;

  PreRanker::Params params;

  if (viewportSearch)
    params.m_minDistanceOnMapBetweenResults = searchParams.m_minDistanceOnMapBetweenResults;

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

namespace
{
class NotInPreffered : public ftypes::BaseChecker
{
  NotInPreffered() : ftypes::BaseChecker(1)
  {
    base::StringIL const types[] = {{"organic"}, {"internet_access"}};
    auto const & c = classif();
    for (auto const & e : types)
      m_types.push_back(c.GetTypeByPath(e));
  }

public:
  DECLARE_CHECKER_INSTANCE(NotInPreffered);
};
}  // namespace

void Processor::InitRanker(Geocoder::Params const & geocoderParams, SearchParams const & searchParams)
{
  bool const viewportSearch = searchParams.m_mode == Mode::Viewport;

  Ranker::Params params;

  params.m_batchSize = searchParams.m_batchSize;
  params.m_limit = searchParams.m_maxNumResults;
  params.m_pivot = GetPivotPoint(viewportSearch);
  {
    storage::CountryInfo info;
    m_infoGetter.GetRegionInfo(params.m_pivot, info);
    params.m_pivotRegion = std::move(info.m_name);
  }

  params.m_preferredTypes = m_preferredTypes;
  // Remove "secondary" category types from preferred.
  base::EraseIf(params.m_preferredTypes, NotInPreffered::Instance());

  params.m_suggestsEnabled = searchParams.m_suggestsEnabled;
  params.m_needAddress = searchParams.m_needAddress;
  params.m_needHighlighting = searchParams.m_needHighlighting && !geocoderParams.IsCategorialRequest();
  params.m_query = m_query;
  params.m_categoryLocales = GetCategoryLocales();
  params.m_viewportSearch = viewportSearch;
  params.m_viewport = GetViewport();
  params.m_categorialRequest = geocoderParams.IsCategorialRequest();

  m_ranker.Init(params, geocoderParams);
}

void Processor::ClearCaches()
{
  m_geocoder.ClearCaches();
  m_localitiesCaches.Clear();
  m_preRanker.ClearCaches();
  m_ranker.ClearCaches();
  m_viewport.MakeEmpty();
}

template <class FnT>
void Processor::EmitResultsFromMwms(std::vector<std::shared_ptr<MwmInfo>> const & infos, FnT const & fn)
{
  // Don't pay attention on possible overhead here, this function is used for debug purpose only.
  std::vector<std::tuple<double, std::string, std::unique_ptr<FeatureType>>> results;
  std::vector<std::unique_ptr<FeaturesLoaderGuard>> guards;

  for (auto const & info : infos)
  {
    auto guard = std::make_unique<FeaturesLoaderGuard>(m_dataSource, MwmSet::MwmId(info));

    fn(*guard, [&](std::unique_ptr<FeatureType> ft)
    {
      // Distance needed for sorting.
      auto const center = feature::GetCenter(*ft, FeatureType::WORST_GEOMETRY);
      double const dist = center.SquaredLength(m_viewport.Center());
      results.emplace_back(dist, guard->GetCountryFileName(), std::move(ft));
    });

    guards.push_back(std::move(guard));
  }

  std::sort(results.begin(), results.end());

  for (auto const & [_, country, ft] : results)
  {
    m_emitter.AddResultNoChecks(
        m_ranker.MakeResult(RankerResult(*ft, country), true /* needAddress */, true /* needHighlighting */));
  }

  m_emitter.Emit();
}
}  // namespace search
