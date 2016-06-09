#include "processor.hpp"

#include "search/common.hpp"
#include "search/dummy_rank_table.hpp"
#include "search/geometry_utils.hpp"
#include "search/intermediate_result.hpp"
#include "search/latlon_match.hpp"
#include "search/locality.hpp"
#include "search/pre_ranking_info.hpp"
#include "search/query_params.hpp"
#include "search/ranking_info.hpp"
#include "search/ranking_utils.hpp"
#include "search/region.hpp"
#include "search/search_index_values.hpp"
#include "search/string_intersection.hpp"

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

#define LONG_OP(op)    \
  {                    \
    if (IsCancelled()) \
      return;          \
    op;                \
  }

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

ftypes::Type GetLocalityIndex(feature::TypesHolder const & types)
{
  using namespace ftypes;

  // Inner logic of SearchAddress expects COUNTRY, STATE and CITY only.
  Type const type = IsLocalityChecker::Instance().GetType(types);
  switch (type)
  {
  case NONE:
  case COUNTRY:
  case STATE:
  case CITY: return type;
  case TOWN: return CITY;
  case VILLAGE: return NONE;
  case LOCALITY_COUNT: return type;
  }
}

class IndexedValue
{
  /// @todo Do not use shared_ptr for optimization issues.
  /// Need to rewrite std::unique algorithm.
  unique_ptr<impl::PreResult2> m_value;

  double m_rank;
  double m_distanceToPivot;

  friend string DebugPrint(IndexedValue const & value)
  {
    ostringstream os;
    os << "IndexedValue [";
    if (value.m_value)
      os << impl::DebugPrint(*value.m_value);
    os << "]";
    return os.str();
  }

public:
  explicit IndexedValue(unique_ptr<impl::PreResult2> value)
    : m_value(move(value)), m_rank(0.0), m_distanceToPivot(numeric_limits<double>::max())
  {
    if (!m_value)
      return;

    auto const & info = m_value->GetRankingInfo();
    m_rank = info.GetLinearModelRank();
    m_distanceToPivot = info.m_distanceToPivot;
  }

  impl::PreResult2 const & operator*() const { return *m_value; }

  inline double GetRank() const { return m_rank; }

  inline double GetDistanceToPivot() const { return m_distanceToPivot; }
};

void RemoveDuplicatingLinear(vector<IndexedValue> & indV)
{
  impl::PreResult2::LessLinearTypesF lessCmp;
  impl::PreResult2::EqualLinearTypesF equalCmp;

  sort(indV.begin(), indV.end(), [&lessCmp](IndexedValue const & lhs, IndexedValue const & rhs)
       {
         return lessCmp(*lhs, *rhs);
       });

  indV.erase(unique(indV.begin(), indV.end(),
                    [&equalCmp](IndexedValue const & lhs, IndexedValue const & rhs)
                    {
                      return equalCmp(*lhs, *rhs);
                    }),
             indV.end());
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

template <typename TSlice>
void UpdateNameScore(string const & name, TSlice const & slice, NameScore & bestScore)
{
  auto const score = GetNameScore(name, slice);
  if (score > bestScore)
    bestScore = score;
}

template <typename TSlice>
void UpdateNameScore(vector<strings::UniString> const & tokens, TSlice const & slice,
                     NameScore & bestScore)
{
  auto const score = GetNameScore(tokens, slice);
  if (score > bestScore)
    bestScore = score;
}

inline bool IsHashtagged(strings::UniString const & s) { return !s.empty() && s[0] == '#'; }

inline strings::UniString RemoveHashtag(strings::UniString const & s)
{
  if (IsHashtagged(s))
    return strings::UniString(s.begin() + 1, s.end());
  return s;
}
}  // namespace

// static
size_t const Processor::kPreResultsCount;

// static
double const Processor::kMinViewportRadiusM = 5.0 * 1000;
double const Processor::kMaxViewportRadiusM = 50.0 * 1000;

Processor::Processor(Index & index, CategoriesHolder const & categories,
                     vector<Suggest> const & suggests,
                     storage::CountryInfoGetter const & infoGetter)
  : m_index(index)
  , m_categories(categories)
  , m_suggests(suggests)
  , m_infoGetter(infoGetter)
#ifdef FIND_LOCALITY_TEST
  , m_locality(&index)
#endif
  , m_position(0, 0)
  , m_mode(Mode::Everywhere)
  , m_worldSearch(true)
  , m_suggestsEnabled(true)
  , m_viewportSearch(false)
  , m_keepHouseNumberInQuery(true)
  , m_preRanker(kPreResultsCount)
  , m_geocoder(index, infoGetter, static_cast<my::Cancellable const &>(*this))
  , m_reverseGeocoder(index)
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
  Reset();

  m_tokens.clear();
  m_prefix.clear();
  m_preRanker.Clear();
  m_viewportSearch = viewportSearch;
}

void Processor::SetViewport(m2::RectD const & viewport, bool forceUpdate)
{
  SetViewportByIndex(viewport, CURRENT_V, forceUpdate);
}

void Processor::SetPreferredLocale(string const & locale)
{
  ASSERT(!locale.empty(), ());

  LOG(LINFO, ("New preffered locale:", locale));

  int8_t const code = StringUtf8Multilang::GetLangIndex(languages::Normalize(locale));
  SetLanguage(LANG_CURRENT, code);

  m_currentLocaleCode = CategoriesHolder::MapLocaleToInteger(locale);

  // Default initialization.
  // If you want to reset input language, call SetInputLocale before search.
  SetInputLocale(locale);

#ifdef FIND_LOCALITY_TEST
  m_locality.SetLanguage(code);
#endif
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
  m_keywordsScorer.SetKeywords(m_tokens.data(), m_tokens.size(), m_prefix);

  // get preffered types to show in results
  m_prefferedTypes.clear();
  ForEachCategoryTypes(QuerySliceOnRawStrings<decltype(m_tokens)>(m_tokens, m_prefix),
                       [&](size_t, uint32_t t)
                       {
                         m_prefferedTypes.insert(t);
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
  m2::RectD const & viewport = m_viewport[CURRENT_V];
  if (viewport.IsPointInside(GetPosition()))
    return GetPosition();
  return viewport.Center();
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

int Processor::GetCategoryLocales(int8_t(&arr)[3]) const
{
  static int8_t const enLocaleCode = CategoriesHolder::MapLocaleToInteger("en");

  // Prepare array of processing locales. English locale is always present for category matching.
  int count = 0;
  if (m_currentLocaleCode != -1)
    arr[count++] = m_currentLocaleCode;
  if (m_inputLocaleCode != -1 && m_inputLocaleCode != m_currentLocaleCode)
    arr[count++] = m_inputLocaleCode;
  if (enLocaleCode != m_currentLocaleCode && enLocaleCode != m_inputLocaleCode)
    arr[count++] = enLocaleCode;

  return count;
}

template <class ToDo>
void Processor::ForEachCategoryTypes(StringSliceBase const & slice, ToDo toDo) const
{
  int8_t arrLocales[3];
  int const localesCount = GetCategoryLocales(arrLocales);

  for (size_t i = 0; i < slice.Size(); ++i)
  {
    auto token = RemoveHashtag(slice.Get(i));
    for (int j = 0; j < localesCount; ++j)
      m_categories.ForEachTypeByName(arrLocales[j], token, bind<void>(ref(toDo), i, _1));
    ProcessEmojiIfNeeded(token, i, toDo);
  }
}

template <class ToDo>
void Processor::ProcessEmojiIfNeeded(strings::UniString const & token, size_t ind,
                                     ToDo & toDo) const
{
  // Special process of 2 codepoints emoji (e.g. black guy on a bike).
  // Only emoji synonyms can have one codepoint.
  if (token.size() > 1)
  {
    static int8_t const enLocaleCode = CategoriesHolder::MapLocaleToInteger("en");

    m_categories.ForEachTypeByName(enLocaleCode, strings::UniString(1, token[0]),
                                   bind<void>(ref(toDo), ind, _1));
  }
}

void Processor::Search(Results & results, size_t limit)
{
  if (m_tokens.empty())
    SuggestStrings(results);

  Geocoder::Params params;

  InitParams(params);
  params.m_mode = m_mode;
  params.m_pivot = GetPivotRect();
  params.m_accuratePivotCenter = GetPivotPoint();
  m_geocoder.SetParams(params);

  m_geocoder.GoEverywhere(m_preRanker);

  FlushResults(params, results, limit);
}

void Processor::SearchViewportPoints(Results & results)
{
  Geocoder::Params params;

  InitParams(params);
  params.m_pivot = m_viewport[CURRENT_V];
  params.m_accuratePivotCenter = params.m_pivot.Center();
  m_geocoder.SetParams(params);

  m_geocoder.GoInViewport(m_preRanker);

  FlushViewportResults(params, results);
}

void Processor::SearchCoordinates(Results & res) const
{
  double lat, lon;
  if (MatchLatLonDegree(m_query, lat, lon))
  {
    ASSERT_EQUAL(res.GetCount(), 0, ());
    res.AddResultNoChecks(MakeResult(impl::PreResult2(lat, lon)));
  }
}

namespace
{
bool IsResultExists(impl::PreResult2 const & p, vector<IndexedValue> const & indV)
{
  impl::PreResult2::StrictEqualF equalCmp(p);
  // Do not insert duplicating results.
  return indV.end() != find_if(indV.begin(), indV.end(), [&equalCmp](IndexedValue const & iv)
                               {
                                 return equalCmp(*iv);
                               });
}
}  // namespace

namespace impl
{
class PreResult2Maker
{
  Processor & m_processor;
  Geocoder::Params const & m_params;

  unique_ptr<Index::FeaturesLoaderGuard> m_pFV;

  // For the best performance, incoming id's should be sorted by id.first (mwm file id).
  void LoadFeature(FeatureID const & id, FeatureType & f, m2::PointD & center, string & name,
                   string & country)
  {
    if (m_pFV.get() == 0 || m_pFV->GetId() != id.m_mwmId)
      m_pFV.reset(new Index::FeaturesLoaderGuard(m_processor.m_index, id.m_mwmId));

    m_pFV->GetFeatureByIndex(id.m_index, f);
    f.SetID(id);

    center = feature::GetCenter(f);

    m_processor.GetBestMatchName(f, name);

    // country (region) name is a file name if feature isn't from World.mwm
    if (m_pFV->IsWorld())
      country.clear();
    else
      country = m_pFV->GetCountryFileName();
  }

  void InitRankingInfo(FeatureType const & ft, m2::PointD const & center,
                       impl::PreResult1 const & res, search::RankingInfo & info)
  {
    auto const & preInfo = res.GetInfo();

    auto const & pivot = m_params.m_accuratePivotCenter;

    info.m_distanceToPivot = MercatorBounds::DistanceOnEarth(center, pivot);
    info.m_rank = preInfo.m_rank;
    info.m_searchType = preInfo.m_searchType;
    info.m_nameScore = NAME_SCORE_ZERO;

    TokenSlice slice(m_params, preInfo.m_startToken, preInfo.m_endToken);
    TokenSliceNoCategories sliceNoCategories(m_params, preInfo.m_startToken, preInfo.m_endToken);

    for (auto const & lang : m_params.m_langs)
    {
      string name;
      if (!ft.GetName(lang, name))
        continue;
      vector<strings::UniString> tokens;
      SplitUniString(NormalizeAndSimplifyString(name), MakeBackInsertFunctor(tokens), Delimiters());

      UpdateNameScore(tokens, slice, info.m_nameScore);
      UpdateNameScore(tokens, sliceNoCategories, info.m_nameScore);
    }

    if (info.m_searchType == SearchModel::SEARCH_TYPE_BUILDING)
      UpdateNameScore(ft.GetHouseNumber(), sliceNoCategories, info.m_nameScore);

    feature::TypesHolder holder(ft);
    vector<pair<size_t, size_t>> matched(slice.Size());
    m_processor.ForEachCategoryTypes(QuerySlice(slice), [&](size_t i, uint32_t t)
                                     {
                                       ++matched[i].second;
                                       if (holder.Has(t))
                                         ++matched[i].first;
                                     });

    info.m_pureCats = all_of(matched.begin(), matched.end(), [](pair<size_t, size_t> const & m)
                             {
                               return m.first != 0;
                             });
    info.m_falseCats = all_of(matched.begin(), matched.end(), [](pair<size_t, size_t> const & m)
                              {
                                return m.first == 0 && m.second != 0;
                              });
  }

  uint8_t NormalizeRank(uint8_t rank, SearchModel::SearchType type, m2::PointD const & center,
                        string const & country)
  {
    switch (type)
    {
    case SearchModel::SEARCH_TYPE_VILLAGE: return rank /= 1.5;
    case SearchModel::SEARCH_TYPE_CITY:
    {
      if (m_processor.GetViewport(Processor::CURRENT_V).IsPointInside(center))
        return rank * 2;

      storage::CountryInfo info;
      if (country.empty())
        m_processor.m_infoGetter.GetRegionInfo(center, info);
      else
        m_processor.m_infoGetter.GetRegionInfo(country, info);
      if (info.IsNotEmpty() && info.m_name == m_processor.GetPivotRegion())
        return rank *= 1.7;
    }
    case SearchModel::SEARCH_TYPE_COUNTRY:
      return rank /= 1.5;

    // For all other search types, rank should be zero for now.
    default: return 0;
    }
  }

public:
  explicit PreResult2Maker(Processor & q, Geocoder::Params const & params)
    : m_processor(q), m_params(params)
  {
  }

  unique_ptr<impl::PreResult2> operator()(impl::PreResult1 const & res1)
  {
    FeatureType ft;
    m2::PointD center;
    string name;
    string country;

    LoadFeature(res1.GetId(), ft, center, name, country);

    auto res2 = make_unique<impl::PreResult2>(ft, &res1, center,
                                              m_processor.GetPosition() /* pivot */, name, country);

    search::RankingInfo info;
    InitRankingInfo(ft, center, res1, info);
    info.m_rank = NormalizeRank(info.m_rank, info.m_searchType, center, country);
    res2->SetRankingInfo(move(info));

    return res2;
  }
};
}  // namespace impl

template <class T>
void Processor::MakePreResult2(Geocoder::Params const & params, vector<T> & cont,
                               vector<FeatureID> & streets)
{
  m_preRanker.Filter(m_viewportSearch);

  // Makes PreResult2 vector.
  impl::PreResult2Maker maker(*this, params);
  m_preRanker.ForEach(
      [&](impl::PreResult1 const & r)
      {
        auto p = maker(r);
        if (!p)
          return;

        if (params.m_mode == Mode::Viewport && !params.m_pivot.IsPointInside(p->GetCenter()))
          return;

        if (p->IsStreet())
          streets.push_back(p->GetID());

        if (!IsResultExists(*p, cont))
          cont.push_back(IndexedValue(move(p)));
      });
}

void Processor::FlushResults(Geocoder::Params const & params, Results & res, size_t resCount)
{
  vector<IndexedValue> indV;
  vector<FeatureID> streets;

  MakePreResult2(params, indV, streets);
  RemoveDuplicatingLinear(indV);
  if (indV.empty())
    return;

  sort(indV.rbegin(), indV.rend(), my::LessBy(&IndexedValue::GetRank));

  ProcessSuggestions(indV, res);

  // Emit feature results.
  size_t count = res.GetCount();
  for (size_t i = 0; i < indV.size() && count < resCount; ++i)
  {
    if (IsCancelled())
      break;

    LOG(LDEBUG, (indV[i]));

    auto const & preResult2 = *indV[i];
    if (res.AddResult(MakeResult(preResult2)))
      ++count;
  }
}

void Processor::FlushViewportResults(Geocoder::Params const & params, Results & res)
{
  vector<IndexedValue> indV;
  vector<FeatureID> streets;

  MakePreResult2(params, indV, streets);
  RemoveDuplicatingLinear(indV);
  if (indV.empty())
    return;

  sort(indV.begin(), indV.end(), my::LessBy(&IndexedValue::GetDistanceToPivot));

  for (size_t i = 0; i < indV.size(); ++i)
  {
    if (IsCancelled())
      break;

    res.AddResultNoChecks(
        (*(indV[i]))
            .GenerateFinalResult(m_infoGetter, &m_categories, &m_prefferedTypes,
                                 m_currentLocaleCode,
                                 nullptr /* Viewport results don't need calculated address */));
  }
}

void Processor::RemoveStringPrefix(string const & str, string & res) const
{
  search::Delimiters delims;
  // Find start iterator of prefix in input query.
  using TIter = utf8::unchecked::iterator<string::const_iterator>;
  TIter iter(str.end());
  while (iter.base() != str.begin())
  {
    TIter prev = iter;
    --prev;

    if (delims(*prev))
      break;
    else
      iter = prev;
  }

  // Assign result with input string without prefix.
  res.assign(str.begin(), iter.base());
}

void Processor::GetSuggestion(string const & name, string & suggest) const
{
  // Splits result's name.
  search::Delimiters delims;
  vector<strings::UniString> tokens;
  SplitUniString(NormalizeAndSimplifyString(name), MakeBackInsertFunctor(tokens), delims);

  // Finds tokens that are already present in the input query.
  vector<bool> tokensMatched(tokens.size());
  bool prefixMatched = false;
  bool fullPrefixMatched = false;

  for (size_t i = 0; i < tokens.size(); ++i)
  {
    auto const & token = tokens[i];

    if (find(m_tokens.begin(), m_tokens.end(), token) != m_tokens.end())
    {
      tokensMatched[i] = true;
    }
    else if (StartsWith(token, m_prefix))
    {
      prefixMatched = true;
      fullPrefixMatched = token.size() == m_prefix.size();
    }
  }

  // When |name| does not match prefix or when prefix equals to some
  // token of the |name| (for example, when user entered "Moscow"
  // without space at the end), we should not suggest anything.
  if (!prefixMatched || fullPrefixMatched)
    return;

  RemoveStringPrefix(m_query, suggest);

  // Appends unmatched result's tokens to the suggestion.
  for (size_t i = 0; i < tokens.size(); ++i)
  {
    if (tokensMatched[i])
      continue;
    suggest.append(strings::ToUtf8(tokens[i]));
    suggest.push_back(' ');
  }
}

template <class T>
void Processor::ProcessSuggestions(vector<T> & vec, Results & res) const
{
  if (m_prefix.empty() || !m_suggestsEnabled)
    return;

  int added = 0;
  for (auto i = vec.begin(); i != vec.end();)
  {
    impl::PreResult2 const & r = **i;

    ftypes::Type const type = GetLocalityIndex(r.GetTypes());
    if ((type == ftypes::COUNTRY || type == ftypes::CITY) || r.IsStreet())
    {
      string suggest;
      GetSuggestion(r.GetName(), suggest);
      if (!suggest.empty() && added < MAX_SUGGESTS_COUNT)
      {
        if (res.AddResult((Result(MakeResult(r), suggest))))
          ++added;

        i = vec.erase(i);
        continue;
      }
    }
    ++i;
  }
}

namespace impl
{
class BestNameFinder
{
  KeywordLangMatcher::ScoreT m_score;
  string & m_name;
  KeywordLangMatcher const & m_keywordsScorer;

public:
  BestNameFinder(string & name, KeywordLangMatcher const & keywordsScorer)
    : m_score(), m_name(name), m_keywordsScorer(keywordsScorer)
  {
  }

  bool operator()(int8_t lang, string const & name)
  {
    KeywordLangMatcher::ScoreT const score = m_keywordsScorer.Score(lang, name);
    if (m_score < score)
    {
      m_score = score;
      m_name = name;
    }
    return true;
  }
};
}  // namespace impl

void Processor::GetBestMatchName(FeatureType const & f, string & name) const
{
  impl::BestNameFinder finder(name, m_keywordsScorer);
  UNUSED_VALUE(f.ForEachName(finder));
}

/// Makes continuous range for tokens and prefix.
template <class TIter, class ValueT>
class CombinedIter
{
  TIter m_i, m_end;
  ValueT const * m_val;

public:
  CombinedIter(TIter i, TIter end, ValueT const * val) : m_i(i), m_end(end), m_val(val) {}

  ValueT const & operator*() const
  {
    ASSERT(m_val != 0 || m_i != m_end, ("dereferencing of empty iterator"));
    if (m_i != m_end)
      return *m_i;

    return *m_val;
  }

  CombinedIter & operator++()
  {
    if (m_i != m_end)
      ++m_i;
    else
      m_val = 0;
    return *this;
  }

  bool operator==(CombinedIter const & other) const
  {
    return m_val == other.m_val && m_i == other.m_i;
  }

  bool operator!=(CombinedIter const & other) const
  {
    return m_val != other.m_val || m_i != other.m_i;
  }
};

class AssignHighlightRange
{
  Result & m_res;

public:
  AssignHighlightRange(Result & res) : m_res(res) {}

  void operator()(pair<uint16_t, uint16_t> const & range) { m_res.AddHighlightRange(range); }
};

Result Processor::MakeResult(impl::PreResult2 const & r) const
{
  Result res = r.GenerateFinalResult(m_infoGetter, &m_categories, &m_prefferedTypes,
                                     m_currentLocaleCode, &m_reverseGeocoder);
  MakeResultHighlight(res);

#ifdef FIND_LOCALITY_TEST
  if (ftypes::IsLocalityChecker::Instance().GetType(r.GetTypes()) == ftypes::NONE)
  {
    string city;
    m_locality.GetLocality(res.GetFeatureCenter(), city);
    res.AppendCity(city);
  }
#endif

  res.SetRankingInfo(r.GetRankingInfo());
  return res;
}

void Processor::MakeResultHighlight(Result & res) const
{
  using TIter = buffer_vector<strings::UniString, 32>::const_iterator;
  using TCombinedIter = CombinedIter<TIter, strings::UniString>;

  TCombinedIter beg(m_tokens.begin(), m_tokens.end(), m_prefix.empty() ? 0 : &m_prefix);
  TCombinedIter end(m_tokens.end(), m_tokens.end(), 0);

  SearchStringTokensIntersectionRanges(res.GetString(), beg, end, AssignHighlightRange(res));
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
  params.Clear();

  if (!m_prefix.empty())
    params.m_prefixTokens.push_back(m_prefix);

  size_t const tokensCount = m_tokens.size();

  // Add normal tokens.
  params.m_tokens.resize(tokensCount);
  for (size_t i = 0; i < tokensCount; ++i)
    params.m_tokens[i].push_back(m_tokens[i]);

  params.m_isCategorySynonym.assign(tokensCount + (m_prefix.empty() ? 0 : 1), false);

  // Add names of categories (and synonyms).
  Classificator const & c = classif();
  auto addSyms = [&](size_t i, uint32_t t)
  {
    QueryParams::TSynonymsVector & v = params.GetTokens(i);

    uint32_t const index = c.GetIndexForType(t);
    v.push_back(FeatureTypeToString(index));
    params.m_isCategorySynonym[i] = true;

    // v2-version MWM has raw classificator types in search index prefix, so
    // do the hack: add synonyms for old convention if needed.
    if (m_supportOldFormat)
    {
      int const type = GetOldTypeFromIndex(index);
      if (type >= 0)
      {
        ASSERT(type == 70 || type > 4000, (type));
        v.push_back(FeatureTypeToString(static_cast<uint32_t>(type)));
      }
    }
  };
  ForEachCategoryTypes(QuerySliceOnRawStrings<decltype(m_tokens)>(m_tokens, m_prefix), addSyms);

  for (auto & tokens : params.m_tokens)
  {
    if (tokens.size() > 1 && IsHashtagged(tokens[0]))
      tokens.erase(tokens.begin());
  }
  if (params.m_prefixTokens.size() > 1 && IsHashtagged(params.m_prefixTokens[0]))
    params.m_prefixTokens.erase(params.m_prefixTokens.begin());

  for (int i = 0; i < LANG_COUNT; ++i)
    params.m_langs.insert(GetLanguage(i));
}

void Processor::ClearCaches()
{
  for (size_t i = 0; i < COUNT_V; ++i)
    ClearCache(i);

  m_locality.ClearCache();
  m_geocoder.ClearCaches();
}

void Processor::SuggestStrings(Results & res)
{
  if (m_prefix.empty() || !m_suggestsEnabled)
    return;
  int8_t arrLocales[3];
  int const localesCount = GetCategoryLocales(arrLocales);

  string prolog;
  RemoveStringPrefix(m_query, prolog);

  for (int i = 0; i < localesCount; ++i)
    MatchForSuggestionsImpl(m_prefix, arrLocales[i], prolog, res);
}

void Processor::MatchForSuggestionsImpl(strings::UniString const & token, int8_t locale,
                                        string const & prolog, Results & res)
{
  for (auto const & suggest : m_suggests)
  {
    strings::UniString const & s = suggest.m_name;
    if ((suggest.m_prefixLength <= token.size()) &&
        (token != s) &&                  // do not push suggestion if it already equals to token
        (suggest.m_locale == locale) &&  // push suggestions only for needed language
        StartsWith(s.begin(), s.end(), token.begin(), token.end()))
    {
      string const utf8Str = strings::ToUtf8(s);
      Result r(utf8Str, prolog + utf8Str + " ");
      MakeResultHighlight(r);
      res.AddResult(move(r));
    }
  }
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
