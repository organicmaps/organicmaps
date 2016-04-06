#include "search_query.hpp"

#include "search/dummy_rank_table.hpp"
#include "search/feature_offset_match.hpp"
#include "search/geometry_utils.hpp"
#include "search/indexed_value.hpp"
#include "search/latlon_match.hpp"
#include "search/locality.hpp"
#include "search/region.hpp"
#include "search/search_common.hpp"
#include "search/search_index_values.hpp"
#include "search/search_query_params.hpp"
#include "search/search_string_intersection.hpp"
#include "search/v2/pre_ranking_info.hpp"
#include "search/v2/ranking_info.hpp"
#include "search/v2/ranking_utils.hpp"

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
#include "std/random.hpp"

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

m2::PointD GetLocalityCenter(Index const & index, MwmSet::MwmId const & id,
                             Locality const & locality)
{
  Index::FeaturesLoaderGuard loader(index, id);
  FeatureType feature;
  loader.GetFeatureByIndex(locality.m_featureId, feature);
  return feature::GetCenter(feature, FeatureType::WORST_GEOMETRY);
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
    case CITY:
      return type;
    case TOWN:
      return CITY;
    case VILLAGE:
      return NONE;
    case LOCALITY_COUNT:
      return type;
  }
}

class IndexedValue : public search::IndexedValueBase<Query::kQueuesCount>
{
  friend string DebugPrint(IndexedValue const & value);

  /// @todo Do not use shared_ptr for optimization issues.
  /// Need to rewrite std::unique algorithm.
  shared_ptr<impl::PreResult2> m_val;

  double m_rank;
  double m_distanceToPivot;

public:
  explicit IndexedValue(unique_ptr<impl::PreResult2> v)
    : m_val(move(v)), m_rank(0), m_distanceToPivot(numeric_limits<double>::max())
  {
    if (!m_val)
      return;

    auto const & info = m_val->GetRankingInfo();
    m_rank = info.GetLinearModelRank();
    m_distanceToPivot = info.m_distanceToPivot;
  }

  impl::PreResult2 const & operator*() const { return *m_val; }

  inline double GetRank() const { return m_rank; }

  inline double GetDistanceToPivot() const { return m_distanceToPivot; }
};

string DebugPrint(IndexedValue const & value)
{
  string index;
  for (auto const & i : value.m_ind)
    index.append(" " + strings::to_string(i));
  return impl::DebugPrint(*value.m_val) + "; Index:" + index;
}

void RemoveDuplicatingLinear(vector<IndexedValue> & indV)
{
  impl::PreResult2::LessLinearTypesF lessCmp;
  impl::PreResult2::EqualLinearTypesF equalCmp;

  sort(indV.begin(), indV.end(),
      [&lessCmp](IndexedValue const & lhs, IndexedValue const & rhs)
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
  m2::RectD minViewport =
      MercatorBounds::RectByCenterXYAndSizeInMeters(viewport.Center(), Query::kMinViewportRadiusM);
  viewport.Add(minViewport);

  m2::RectD maxViewport =
      MercatorBounds::RectByCenterXYAndSizeInMeters(viewport.Center(), Query::kMaxViewportRadiusM);
  VERIFY(viewport.Intersect(maxViewport), ());
  return viewport;
}

m2::RectD GetRectAroundPosition(m2::PointD const & position)
{
  double constexpr kMaxPositionRadiusM = 50.0 * 1000;
  return MercatorBounds::RectByCenterXYAndSizeInMeters(position, kMaxPositionRadiusM);
}

template <typename TSlice>
void UpdateNameScore(string const & name, TSlice const & slice, v2::NameScore & bestScore)
{
  auto const score = v2::GetNameScore(name, slice);
  if (score > bestScore)
    bestScore = score;
}

template <typename TSlice>
void UpdateNameScore(vector<strings::UniString> const & tokens, TSlice const & slice,
                     v2::NameScore & bestScore, double & bestCoverage)
{
  auto const score = v2::GetNameScore(tokens, slice);
  auto const coverage =
      tokens.empty() ? 0 : static_cast<double>(slice.Size()) / static_cast<double>(tokens.size());
  if (score > bestScore)
  {
    bestScore = score;
    bestCoverage = coverage;
  }
  else if (score == bestScore && coverage > bestCoverage)
  {
    bestCoverage = coverage;
  }
}
}  // namespace

// static
size_t const Query::kPreResultsCount;

// static
double const Query::kMinViewportRadiusM = 5.0 * 1000;
double const Query::kMaxViewportRadiusM = 50.0 * 1000;

Query::Query(Index & index, CategoriesHolder const & categories, vector<Suggest> const & suggests,
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
  , m_keepHouseNumberInQuery(false)
  , m_reverseGeocoder(index)
{
  // Initialize keywords scorer.
  // Note! This order should match the indexes arrays above.
  vector<vector<int8_t> > langPriorities = {
    {-1}, // future current lang
    {-1}, // future input lang
    {StringUtf8Multilang::kInternationalCode, StringUtf8Multilang::kEnglishCode},
    {StringUtf8Multilang::kDefaultCode}
  };

  m_keywordsScorer.SetLanguages(langPriorities);

  SetPreferredLocale("en");
}

void Query::SetLanguage(int id, int8_t lang)
{
  m_keywordsScorer.SetLanguage(GetLangIndex(id), lang);
}

int8_t Query::GetLanguage(int id) const
{
  return m_keywordsScorer.GetLanguage(GetLangIndex(id));
}

m2::PointD Query::GetPivotPoint() const
{
  m2::RectD const & viewport = m_viewport[CURRENT_V];
  if (viewport.IsPointInside(GetPosition()))
    return GetPosition();
  return viewport.Center();
}

m2::RectD Query::GetPivotRect() const
{
  m2::RectD const & viewport = m_viewport[CURRENT_V];
  if (viewport.IsPointInside(GetPosition()))
    return GetRectAroundPosition(GetPosition());
  return NormalizeViewport(viewport);
}

void Query::SetViewport(m2::RectD const & viewport, bool forceUpdate)
{
  Reset();

  TMWMVector mwmsInfo;
  m_index.GetMwmsInfo(mwmsInfo);

  SetViewportByIndex(mwmsInfo, viewport, CURRENT_V, forceUpdate);
}

void Query::SetViewportByIndex(TMWMVector const & mwmsInfo, m2::RectD const & viewport, size_t idx,
                               bool forceUpdate)
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

void Query::SetRankPivot(m2::PointD const & pivot)
{
  if (!m2::AlmostEqualULPs(pivot, m_pivot))
  {
    storage::CountryInfo ci;
    m_infoGetter.GetRegionInfo(pivot, ci);
    m_region.swap(ci.m_name);
  }

  m_pivot = pivot;
}

void Query::SetPreferredLocale(string const & locale)
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

void Query::SetInputLocale(string const & locale)
{
  if (!locale.empty())
  {
    LOG(LDEBUG, ("New input locale:", locale));

    SetLanguage(LANG_INPUT, StringUtf8Multilang::GetLangIndex(languages::Normalize(locale)));

    m_inputLocaleCode = CategoriesHolder::MapLocaleToInteger(locale);
  }
}

void Query::ClearCaches()
{
  for (size_t i = 0; i < COUNT_V; ++i)
    ClearCache(i);

  m_locality.ClearCache();
}

void Query::ClearCache(size_t ind)
{
  m_viewport[ind].MakeEmpty();
}

void Query::Init(bool viewportSearch)
{
  Reset();

  m_tokens.clear();
  m_prefix.clear();
  m_viewportSearch = viewportSearch;

  ClearResults();
}

void Query::ClearResults()
{
  m_results.clear();
}

int Query::GetCategoryLocales(int8_t (&arr) [3]) const
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
void Query::ForEachCategoryTypes(ToDo toDo) const
{
  int8_t arrLocales[3];
  int const localesCount = GetCategoryLocales(arrLocales);

  size_t const tokensCount = m_tokens.size();
  for (size_t i = 0; i < tokensCount; ++i)
  {
    for (int j = 0; j < localesCount; ++j)
      m_categories.ForEachTypeByName(arrLocales[j], m_tokens[i], bind<void>(ref(toDo), i, _1));

    ProcessEmojiIfNeeded(m_tokens[i], i, toDo);
  }

  if (!m_prefix.empty())
  {
    for (int j = 0; j < localesCount; ++j)
    {
      m_categories.ForEachTypeByName(arrLocales[j], m_prefix,
                                     bind<void>(ref(toDo), tokensCount, _1));
    }

    ProcessEmojiIfNeeded(m_prefix, tokensCount, toDo);
  }
}

template <class ToDo>
void Query::ProcessEmojiIfNeeded(strings::UniString const & token, size_t ind, ToDo & toDo) const
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

void Query::SetQuery(string const & query)
{
  m_query = query;

  /// @todo Why Init is separated with SetQuery?
  ASSERT(m_tokens.empty(), ());
  ASSERT(m_prefix.empty(), ());

  // Split input query by tokens with possible last prefix.
  search::Delimiters delims;
  SplitUniString(NormalizeAndSimplifyString(query), MakeBackInsertFunctor(m_tokens), delims);

  bool checkPrefix = true;

  // Assign prefix with last parsed token.
  if (checkPrefix && !m_tokens.empty() && !delims(strings::LastUniChar(query)))
  {
    m_prefix.swap(m_tokens.back());
    m_tokens.pop_back();
  }

  int const maxTokensCount = MAX_TOKENS-1;
  if (m_tokens.size() > maxTokensCount)
    m_tokens.resize(maxTokensCount);

  // assign tokens and prefix to scorer
  m_keywordsScorer.SetKeywords(m_tokens.data(), m_tokens.size(), m_prefix);

  // get preffered types to show in results
  m_prefferedTypes.clear();
  ForEachCategoryTypes([&] (size_t, uint32_t t)
  {
    m_prefferedTypes.insert(t);
  });
}

void Query::Search(Results & res, size_t resCount)
{
  if (IsCancelled())
    return;

  if (m_tokens.empty())
    SuggestStrings(res);

  LONG_OP(SearchAddress(res));
  LONG_OP(SearchFeatures());

  // TODO (@y): this code is not working and will gone away.
  v2::Geocoder::Params params;
  LONG_OP(FlushResults(params, res, false /* allMWMs */, resCount, true /* oldHouseSearch */));
}

void Query::SearchViewportPoints(Results & res)
{
  LONG_OP(SearchAddress(res));
  LONG_OP(SearchFeaturesInViewport(CURRENT_V));

  // TODO (@y): this code is not working and will gone away.
  v2::Geocoder::Params params;
  FlushViewportResults(params, res, true /* oldHouseSearch */);
}

void Query::FlushViewportResults(v2::Geocoder::Params const & params, Results & res,
                                 bool oldHouseSearch)
{
  vector<IndexedValue> indV;
  vector<FeatureID> streets;

  MakePreResult2(params, indV, streets);
  RemoveDuplicatingLinear(indV);
  if (indV.empty())
    return;

  sort(indV.begin(), indV.end(), my::CompareBy(&IndexedValue::GetDistanceToPivot));

  for (size_t i = 0; i < indV.size(); ++i)
  {
    if (IsCancelled())
      break;

    res.AddResultNoChecks((*(indV[i])).GenerateFinalResult(m_infoGetter, &m_categories,
        &m_prefferedTypes, m_currentLocaleCode,
        nullptr /* Viewport results don't need calculated address */));
  }
}

void Query::SearchCoordinates(Results & res) const
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
struct LessFeatureID
{
  using TValue = impl::PreResult1;

  inline bool operator()(TValue const & lhs, TValue const & rhs) const
  {
    return lhs.GetID() < rhs.GetID();
  }
};

struct EqualFeatureID
{
  using TValue = impl::PreResult1;

  inline bool operator()(TValue const & lhs, TValue const & rhs) const
  {
    return lhs.GetID() == rhs.GetID();
  }
};

// Orders PreResult1 by following criterion:
// 1. Feature Id (increasing), if same...
// 2. Number of matched tokens from the query (decreasing), if same...
// 3. Index of the first matched token from the query (increasing).
struct ComparePreResult1
{
  bool operator()(impl::PreResult1 const & lhs, impl::PreResult1 const & rhs) const
  {
    if (lhs.GetID() != rhs.GetID())
      return lhs.GetID() < rhs.GetID();

    auto const & linfo = lhs.GetInfo();
    auto const & rinfo = rhs.GetInfo();
    if (linfo.GetNumTokens() != rinfo.GetNumTokens())
      return linfo.GetNumTokens() > rinfo.GetNumTokens();
    return linfo.m_startToken < rinfo.m_startToken;
  }
};

void RemoveDuplicatingPreResults1(vector<impl::PreResult1> & results)
{
  sort(results.begin(), results.end(), ComparePreResult1());
  results.erase(unique(results.begin(), results.end(), EqualFeatureID()), results.end());
}

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
  Query & m_query;
  v2::Geocoder::Params const & m_params;

  unique_ptr<Index::FeaturesLoaderGuard> m_pFV;

  // For the best performance, incoming id's should be sorted by id.first (mwm file id).
  void LoadFeature(FeatureID const & id, FeatureType & f, m2::PointD & center, string & name,
                   string & country)
  {
    if (m_pFV.get() == 0 || m_pFV->GetId() != id.m_mwmId)
      m_pFV.reset(new Index::FeaturesLoaderGuard(m_query.m_index, id.m_mwmId));

    m_pFV->GetFeatureByIndex(id.m_index, f);
    f.SetID(id);

    center = feature::GetCenter(f);

    m_query.GetBestMatchName(f, name);

    // country (region) name is a file name if feature isn't from World.mwm
    if (m_pFV->IsWorld())
      country.clear();
    else
      country = m_pFV->GetCountryFileName();
  }

  void InitRankingInfo(FeatureType const & ft, m2::PointD const & center,
                       impl::PreResult1 const & res, search::v2::RankingInfo & info)
  {
    auto const & preInfo = res.GetInfo();

    auto const & pivot = m_params.m_accuratePivotCenter;

    info.m_distanceToPivot = MercatorBounds::DistanceOnEarth(center, pivot);
    info.m_rank = preInfo.m_rank;
    info.m_searchType = preInfo.m_searchType;

    info.m_nameScore = v2::NAME_SCORE_ZERO;

    v2::TokensSliceNoCategories slice(m_params, preInfo.m_startToken, preInfo.m_endToken);

    for (auto const & lang : m_params.m_langs)
    {
      string name;
      if (!ft.GetName(lang, name))
        continue;
      vector<strings::UniString> tokens;
      SplitUniString(NormalizeAndSimplifyString(name), MakeBackInsertFunctor(tokens), Delimiters());

      UpdateNameScore(tokens, slice, info.m_nameScore, info.m_nameCoverage);
    }

    if (info.m_searchType == v2::SearchModel::SEARCH_TYPE_BUILDING)
      UpdateNameScore(ft.GetHouseNumber(), slice, info.m_nameScore);
  }

  uint8_t NormalizeRank(uint8_t rank, v2::SearchModel::SearchType type, m2::PointD const & center,
                        string const & country)
  {
    switch (type)
    {
    case v2::SearchModel::SEARCH_TYPE_VILLAGE: return rank /= 1.5;
    case v2::SearchModel::SEARCH_TYPE_CITY:
    {
      if (m_query.GetViewport(Query::CURRENT_V).IsPointInside(center))
        return rank * 2;

      storage::CountryInfo info;
      if (country.empty())
        m_query.m_infoGetter.GetRegionInfo(center, info);
      else
        m_query.m_infoGetter.GetRegionInfo(country, info);
      if (info.IsNotEmpty() && info.m_name == m_query.GetPivotRegion())
        return rank *= 1.7;
    }
    case v2::SearchModel::SEARCH_TYPE_COUNTRY: return rank /= 1.5;
    default: return rank;
    }
  }

public:
  explicit PreResult2Maker(Query & q, v2::Geocoder::Params const & params)
    : m_query(q), m_params(params)
  {
  }

  unique_ptr<impl::PreResult2> operator()(impl::PreResult1 const & res1)
  {
    FeatureType ft;
    m2::PointD center;
    string name;
    string country;

    LoadFeature(res1.GetID(), ft, center, name, country);

    auto res2 = make_unique<impl::PreResult2>(ft, &res1, center, m_query.GetPosition() /* pivot */,
                                              name, country);

    search::v2::RankingInfo info;
    InitRankingInfo(ft, center, res1, info);
    info.m_rank = NormalizeRank(info.m_rank, info.m_searchType, center, country);
    res2->SetRankingInfo(move(info));

    return res2;
  }
};
}  // namespace impl

template <class T>
void Query::MakePreResult2(v2::Geocoder::Params const & params, vector<T> & cont,
                           vector<FeatureID> & streets)
{
  // make unique set of PreResult1
  using TPreResultSet = set<impl::PreResult1, LessFeatureID>;
  TPreResultSet theSet;

  RemoveDuplicatingPreResults1(m_results);

  sort(m_results.begin(), m_results.end(), &impl::PreResult1::LessPriority);
  if (kPreResultsCount != 0 && m_results.size() > kPreResultsCount)
  {
    // Priority is some kind of distance from the viewport or
    // position, therefore if we have a bunch of results with the same
    // priority, we have no idea here which results are relevant.  To
    // prevent bias from previous search routines (like sorting by
    // feature id) this code randomly selects tail of the
    // sorted-by-priority list of pre-results.

    double const lastPriority = m_results[kPreResultsCount - 1].GetPriority();

    auto b = m_results.begin() + kPreResultsCount - 1;
    for (; b != m_results.begin() && b->GetPriority() == lastPriority; --b)
      ;
    if (b->GetPriority() != lastPriority)
      ++b;

    auto e = m_results.begin() + kPreResultsCount;
    for (; e != m_results.end() && e->GetPriority() == lastPriority; ++e)
      ;

    // The main reason of shuffling here is to select a random subset
    // from the low-priority results. We're using a linear
    // congruential method with default seed because it is fast,
    // simple and doesn't need an external entropy source.
    //
    // TODO (@y, @m, @vng): consider to take some kind of hash from
    // features and then select a subset with smallest values of this
    // hash.  In this case this subset of results will be persistent
    // to small changes in the original set.
    minstd_rand engine;
    shuffle(b, e, engine);
  }
  theSet.insert(m_results.begin(), m_results.begin() + min(m_results.size(), kPreResultsCount));

  if (!m_viewportSearch)
  {
    size_t n = min(m_results.size(), kPreResultsCount);
    nth_element(m_results.begin(), m_results.begin() + n, m_results.end(),
                &impl::PreResult1::LessRank);
    theSet.insert(m_results.begin(), m_results.begin() + n);
  }

  ClearResults();

  // Makes PreResult2 vector.
  impl::PreResult2Maker maker(*this, params);
  for (auto const & r : theSet)
  {
    auto p = maker(r);
    if (!p)
      continue;

    if (params.m_mode == Mode::Viewport && !params.m_pivot.IsPointInside(p->GetCenter()))
      continue;

    if (p->IsStreet())
      streets.push_back(p->GetID());

    if (!IsResultExists(*p, cont))
      cont.push_back(IndexedValue(move(p)));
  }
}

void Query::FlushResults(v2::Geocoder::Params const & params, Results & res, bool allMWMs,
                         size_t resCount, bool oldHouseSearch)
{
  vector<IndexedValue> indV;
  vector<FeatureID> streets;

  MakePreResult2(params, indV, streets);
  RemoveDuplicatingLinear(indV);
  if (indV.empty())
    return;

  sort(indV.rbegin(), indV.rend(), my::CompareBy(&IndexedValue::GetRank));

  // Do not process suggestions in additional search.
  if (!allMWMs || res.GetCount() == 0)
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

int Query::GetQueryIndexScale(m2::RectD const & viewport) const
{
  return search::GetQueryIndexScale(viewport);
}

void Query::RemoveStringPrefix(string const & str, string & res) const
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

void Query::GetSuggestion(string const & name, string & suggest) const
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
      tokensMatched[i] = true;
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
void Query::ProcessSuggestions(vector<T> & vec, Results & res) const
{
  if (m_prefix.empty() || !m_suggestsEnabled)
    return;

  int added = 0;
  for (auto i = vec.begin(); i != vec.end();)
  {
    impl::PreResult2 const & r = **i;

    ftypes::Type const type = GetLocalityIndex(r.GetTypes());
    if ((type == ftypes::COUNTRY || type == ftypes::CITY)  || r.IsStreet())
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

void Query::AddPreResult1(MwmSet::MwmId const & mwmId, uint32_t featureId, double priority,
                          v2::PreRankingInfo const & info, ViewportID viewportId /* = DEFAULT_V */)
{
  FeatureID id(mwmId, featureId);
  m_results.emplace_back(id, priority, viewportId, info);
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

void Query::GetBestMatchName(FeatureType const & f, string & name) const
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
    ASSERT( m_val != 0 || m_i != m_end, ("dereferencing of empty iterator") );
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
  AssignHighlightRange(Result & res)
    : m_res(res)
  {
  }

  void operator() (pair<uint16_t, uint16_t> const & range)
  {
    m_res.AddHighlightRange(range);
  }
};


Result Query::MakeResult(impl::PreResult2 const & r) const
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

void Query::MakeResultHighlight(Result & res) const
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
}

void Query::InitParams(bool localitySearch, SearchQueryParams & params)
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
  if (!localitySearch)
  {
    Classificator const & cl = classif();
    auto addSyms = [&](size_t i, uint32_t t)
    {
      SearchQueryParams::TSynonymsVector & v =
          (i < tokensCount ? params.m_tokens[i] : params.m_prefixTokens);

      uint32_t const index = cl.GetIndexForType(t);
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
    ForEachCategoryTypes(addSyms);
  }

  for (int i = 0; i < LANG_COUNT; ++i)
    params.m_langs.insert(GetLanguage(i));
}

void Query::SearchAddress(Results & res)
{
  // Find World.mwm and do special search there.
  TMWMVector mwmsInfo;
  m_index.GetMwmsInfo(mwmsInfo);

  for (shared_ptr<MwmInfo> & info : mwmsInfo)
  {
    MwmSet::MwmId mwmId(info);
    Index::MwmHandle const mwmHandle = m_index.GetMwmHandleById(mwmId);
    MwmValue const * pMwm = mwmHandle.GetValue<MwmValue>();
    if (pMwm && pMwm->m_cont.IsExist(SEARCH_INDEX_FILE_TAG) &&
        pMwm->GetHeader().GetType() == TFHeader::world)
    {
      Locality city;
      Region region;
      SearchLocality(pMwm, city, region);

      if (city.IsValid())
      {
        LOG(LDEBUG, ("Final city-locality = ", city));

        SearchQueryParams params;
        InitParams(false /* localitySearch */, params);
        params.EraseTokens(city.m_matchedTokens);

        if (params.CanSuggest())
          SuggestStrings(res);

        if (!params.IsEmpty())
        {
          params.ProcessAddressTokens();

          m2::PointD const cityCenter = GetLocalityCenter(m_index, mwmId, city);
          double const cityRadius = city.m_radius;
          m2::RectD const rect =
              MercatorBounds::RectByCenterXYAndSizeInMeters(cityCenter, cityRadius);
          SetViewportByIndex(mwmsInfo, rect, LOCALITY_V, false /* forceUpdate */);

          /// @todo Hack - do not search for address in World.mwm; Do it better in future.
          bool const b = m_worldSearch;
          m_worldSearch = false;
          MY_SCOPE_GUARD(restoreWorldSearch, [&]() { m_worldSearch = b; });

          // Candidates for search around locality. Initially filled
          // with mwms containing city center.
          TMWMVector localityMwms;
          string const localityFile = m_infoGetter.GetRegionCountryId(cityCenter);
          auto localityMismatch = [&localityFile](shared_ptr<MwmInfo> const & info)
          {
            return info->GetCountryName() != localityFile;
          };
          remove_copy_if(mwmsInfo.begin(), mwmsInfo.end(), back_inserter(localityMwms),
                         localityMismatch);
          SearchFeaturesInViewport(params, localityMwms, LOCALITY_V);
        }
        else
        {
          // Add found locality as a result if nothing left to match.

          // TODO (@y): for backward compatibility with old search
          // algorithm carefully fill |info| here.
          v2::PreRankingInfo info;
          info.m_rank = city.m_rank;

          AddPreResult1(mwmId, city.m_featureId, 1.0 /* priority */, info);
        }
      }
      else if (region.IsValid())
      {
        LOG(LDEBUG, ("Final region-locality = ", region));

        SearchQueryParams params;
        InitParams(false /* localitySearch */, params);
        params.EraseTokens(region.m_matchedTokens);

        if (params.CanSuggest())
          SuggestStrings(res);

        if (!params.IsEmpty())
        {
          TMWMVector regionMwms;
          auto regionMismatch = [this, &region](shared_ptr<MwmInfo> const & info)
          {
            return !m_infoGetter.IsBelongToRegions(info->GetCountryName(), region.m_ids);
          };
          remove_copy_if(mwmsInfo.begin(), mwmsInfo.end(), back_inserter(regionMwms),
                         regionMismatch);
          SearchInMwms(regionMwms, params, DEFAULT_V);
        }
      }

      return;
    }
  }
}

namespace impl
{
class DoFindLocality
{
  Query const & m_query;

  /// Index in array equal to Locality::m_type value.
  vector<Locality> m_localities[3];

  FeaturesVector m_vector;
  unique_ptr<RankTable> m_table;
  size_t m_index;  ///< index of processing token

  int8_t m_lang;
  int8_t m_arrEn[3];

  /// Tanslates country full english name to mwm file name prefix
  /// (need when matching correspondent mwm file in CountryInfoGetter::GetMatchedRegions).
  //@{
  static bool FeatureName2FileNamePrefix(string & name, char const * prefix, char const * arr[],
                                         size_t n)
  {
    for (size_t i = 0; i < n; ++i)
      if (name.find(arr[i]) == string::npos)
        return false;

    name = prefix;
    return true;
  }

  void AssignEnglishName(FeatureType const & f, Locality & l)
  {
    // search for name in next order: "en", "int_name", "default"
    for (int i = 0; i < 3; ++i)
      if (f.GetName(m_arrEn[i], l.m_enName))
      {
        // make name lower-case
        strings::AsciiToLower(l.m_enName);

        char const * arrUSA[] = {"united", "states", "america"};
        char const * arrUK[] = {"united", "kingdom"};

        if (!FeatureName2FileNamePrefix(l.m_enName, "usa", arrUSA, ARRAY_SIZE(arrUSA)))
          if (!FeatureName2FileNamePrefix(l.m_enName, "uk", arrUK, ARRAY_SIZE(arrUK)))
            return;
      }
  }
  //@}

  void AddRegions(int index, vector<Region> & regions) const
  {
    // fill regions vector in priority order
    vector<Locality> const & arr = m_localities[index];
    for (auto i = arr.rbegin(); i != arr.rend(); ++i)
    {
      // no need to check region with empty english name (can't match for polygon)
      if (!i->m_enName.empty() && i->IsSuitable(m_query.m_tokens, m_query.m_prefix))
      {
        vector<size_t> vec;
        m_query.m_infoGetter.GetMatchedRegions(i->m_enName, vec);
        if (!vec.empty())
        {
          regions.push_back(Region());
          Region & r = regions.back();

          r.m_ids.swap(vec);
          r.m_matchedTokens = i->m_matchedTokens;
          r.m_enName = i->m_enName;
        }
      }
    }
  }

  bool InRegion(Locality const & loc, Region const & r) const
  {
    // check that locality and region are produced from different tokens
    vector<size_t> dummy;
    set_intersection(loc.m_matchedTokens.begin(), loc.m_matchedTokens.end(),
                     r.m_matchedTokens.begin(), r.m_matchedTokens.end(), back_inserter(dummy));

    if (dummy.empty())
    {
      // check that locality belong to region
      return m_query.m_infoGetter.IsBelongToRegions(loc.m_center, r.m_ids);
    }

    return false;
  }

  class EqualID
  {
    uint32_t m_id;

  public:
    EqualID(uint32_t id) : m_id(id) {}

    bool operator()(Locality const & l) const { return (l.m_featureId == m_id); }
  };

public:
  DoFindLocality(Query & q, MwmValue const * pMwm, int8_t lang)
    : m_query(q)
    , m_vector(pMwm->m_cont, pMwm->GetHeader(), pMwm->m_table)
    , m_table(RankTable::Load(pMwm->m_cont))
    , m_lang(lang)
  {
    m_arrEn[0] = q.GetLanguage(LANG_EN);
    m_arrEn[1] = q.GetLanguage(LANG_INTERNATIONAL);
    m_arrEn[2] = q.GetLanguage(LANG_DEFAULT);
  }

  void Resize(size_t) {}

  void SwitchTo(size_t ind) { m_index = ind; }

  void operator()(FeatureWithRankAndCenter const & value) { operator()(value.m_featureId); }

  void operator()(FeatureIndexValue const & value) { operator()(value.m_featureId); }

  void operator()(uint32_t const featureId)
  {
    if (m_query.IsCancelled())
      throw Query::CancelException();

    // find locality in current results
    for (size_t i = 0; i < 3; ++i)
    {
      auto it = find_if(m_localities[i].begin(), m_localities[i].end(), EqualID(featureId));
      if (it != m_localities[i].end())
      {
        it->m_matchedTokens.push_back(m_index);
        return;
      }
    }

    // Load feature.
    FeatureType f;
    m_vector.GetByIndex(featureId, f);

    using namespace ftypes;

    // Check, if feature is locality.
    Type const type = GetLocalityIndex(feature::TypesHolder(f));
    if (type == NONE)
      return;
    ASSERT_LESS_OR_EQUAL(0, type, ());
    ASSERT_LESS(type, ARRAY_SIZE(m_localities), ());

    m2::PointD const center = feature::GetCenter(f, FeatureType::WORST_GEOMETRY);
    uint8_t rank = 0;
    if (m_table.get())
    {
      ASSERT_LESS(featureId, m_table->Size(), ());
      rank = m_table->Get(featureId);
    }
    else
    {
      LOG(LWARNING, ("Can't get ranks table for locality search."));
    }
    m_localities[type].emplace_back(type, featureId, center, rank);
    Locality & loc = m_localities[type].back();

    loc.m_radius = GetRadiusByPopulation(GetPopulation(f));
    // m_lang name should exist if we matched feature in search index for this language.
    VERIFY(f.GetName(m_lang, loc.m_name), ());
    loc.m_matchedTokens.push_back(m_index);
    AssignEnglishName(f, loc);
  }

  void SortLocalities()
  {
    for (int i = ftypes::COUNTRY; i <= ftypes::CITY; ++i)
      sort(m_localities[i].begin(), m_localities[i].end());
  }

  void GetRegions(vector<Region> & regions) const
  {
    // LOG(LDEBUG, ("Countries before processing = ", m_localities[ftypes::COUNTRY]));
    // LOG(LDEBUG, ("States before processing = ", m_localities[ftypes::STATE]));

    AddRegions(ftypes::STATE, regions);
    AddRegions(ftypes::COUNTRY, regions);

    // LOG(LDEBUG, ("Regions after processing = ", regions));
  }

  void GetBestCity(Locality & res, vector<Region> const & regions)
  {
    size_t const regsCount = regions.size();
    vector<Locality> & arr = m_localities[ftypes::CITY];

    // Interate in reverse order from better to generic locality.
    for (auto i = arr.rbegin(); i != arr.rend(); ++i)
    {
      if (!i->IsSuitable(m_query.m_tokens, m_query.m_prefix))
        continue;

      // additional check for locality belongs to region
      vector<Region const *> belongs;
      for (size_t j = 0; j < regsCount; ++j)
      {
        if (InRegion(*i, regions[j]))
          belongs.push_back(&regions[j]);
      }

      for (size_t j = 0; j < belongs.size(); ++j)
      {
        // splice locality info with region info
        i->m_matchedTokens.insert(i->m_matchedTokens.end(), belongs[j]->m_matchedTokens.begin(),
                                  belongs[j]->m_matchedTokens.end());
        // we need to store sorted range of token indexies
        sort(i->m_matchedTokens.begin(), i->m_matchedTokens.end());

        i->m_enName += (", " + belongs[j]->m_enName);
      }

      if (res < *i)
        i->Swap(res);

      if (regsCount == 0)
        return;
    }
  }
};

}  // namespace impl

namespace
{
template <typename TValue>
void SearchLocalityImpl(Query * query, MwmValue const * pMwm, Locality & res1, Region & res2,
                        SearchQueryParams & params, serial::CodingParams & codingParams)
{
    ModelReaderPtr searchReader = pMwm->m_cont.GetReader(SEARCH_INDEX_FILE_TAG);

    auto const trieRoot = trie::ReadTrie<SubReaderWrapper<Reader>, ValueList<TValue>>(
        SubReaderWrapper<Reader>(searchReader.GetPtr()),
        SingleValueSerializer<TValue>(codingParams));

    auto finder = [&](TrieRootPrefix<TValue> & langRoot, int8_t lang)
    {
      impl::DoFindLocality doFind(*query, pMwm, lang);
      MatchTokensInTrie(params.m_tokens, langRoot, doFind);

      // Last token's prefix is used as a complete token here, to limit number of
      // results.
      doFind.Resize(params.m_tokens.size() + 1);
      doFind.SwitchTo(params.m_tokens.size());
      MatchTokenInTrie(params.m_prefixTokens, langRoot, doFind);
      doFind.SortLocalities();

      // Get regions from STATE and COUNTRY localities
      vector<Region> regions;
      doFind.GetRegions(regions);

      // Get best CITY locality.
      Locality loc;
      doFind.GetBestCity(loc, regions);
      if (res1 < loc)
      {
        LOG(LDEBUG, ("Better location ", loc, " for language ", lang));
        res1.Swap(loc);
      }

      // Get best region.
      if (!regions.empty())
      {
        sort(regions.begin(), regions.end());
        if (res2 < regions.back())
          res2.Swap(regions.back());
      }
    };

    ForEachLangPrefix(params, *trieRoot, finder);
}
}  // namespace

void Query::SearchLocality(MwmValue const * pMwm, Locality & res1, Region & res2)
{
  SearchQueryParams params;
  InitParams(true /* localitySearch */, params);

  auto codingParams = trie::GetCodingParams(pMwm->GetHeader().GetDefCodingParams());

  version::MwmTraits mwmTraits(pMwm->GetHeader().GetFormat());

  if (mwmTraits.GetSearchIndexFormat() ==
      version::MwmTraits::SearchIndexFormat::FeaturesWithRankAndCenter)
  {
    using TValue = FeatureWithRankAndCenter;
    SearchLocalityImpl<TValue>(this, pMwm, res1, res2, params, codingParams);
  }
  else if (mwmTraits.GetSearchIndexFormat() ==
           version::MwmTraits::SearchIndexFormat::CompressedBitVector)
  {
    using TValue = FeatureIndexValue;
    SearchLocalityImpl<TValue>(this, pMwm, res1, res2, params, codingParams);
  }
}

void Query::SearchFeatures()
{
  TMWMVector mwmsInfo;
  m_index.GetMwmsInfo(mwmsInfo);

  SearchQueryParams params;
  InitParams(false /* localitySearch */, params);

  SearchInMwms(mwmsInfo, params, CURRENT_V);
}

void Query::SearchFeaturesInViewport(ViewportID viewportId)
{
  TMWMVector mwmsInfo;
  m_index.GetMwmsInfo(mwmsInfo);

  SearchQueryParams params;
  InitParams(false /* localitySearch */, params);

  SearchFeaturesInViewport(params, mwmsInfo, viewportId);
}

void Query::SearchFeaturesInViewport(SearchQueryParams const & params, TMWMVector const & mwmsInfo,
                                     ViewportID viewportId)
{
  m2::RectD const * viewport = nullptr;
  if (viewportId == LOCALITY_V)
    viewport = &m_viewport[LOCALITY_V];
  else
    viewport = &m_viewport[CURRENT_V];
  if (!viewport->IsValid())
    return;

  TMWMVector viewportMwms;
  auto viewportMispatch = [&viewport](shared_ptr<MwmInfo> const & info)
  {
    return !viewport->IsIntersect(info->m_limitRect);
  };
  remove_copy_if(mwmsInfo.begin(), mwmsInfo.end(), back_inserter(viewportMwms), viewportMispatch);
  SearchInMwms(viewportMwms, params, viewportId);
}

void Query::SearchInMwms(TMWMVector const & mwmsInfo, SearchQueryParams const & params,
                         ViewportID viewportId)
{
  /// @note Old Retrieval logic is obsolete and moved to trash.
  /// @todo Move to trash this Query code :)
}

void Query::SuggestStrings(Results & res)
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

void Query::MatchForSuggestionsImpl(strings::UniString const & token, int8_t locale,
                                    string const & prolog, Results & res)
{
  for (auto const & suggest : m_suggests)
  {
    strings::UniString const & s = suggest.m_name;
    if ((suggest.m_prefixLength <= token.size()) &&
        (token != s) &&                   // do not push suggestion if it already equals to token
        (suggest.m_locale == locale) &&   // push suggestions only for needed language
        StartsWith(s.begin(), s.end(), token.begin(), token.end()))
    {
      string const utf8Str = strings::ToUtf8(s);
      Result r(utf8Str, prolog + utf8Str + " ");
      MakeResultHighlight(r);
      res.AddResult(move(r));
    }
  }
}

m2::RectD const & Query::GetViewport(ViewportID vID /*= DEFAULT_V*/) const
{
  if (vID == LOCALITY_V)
  {
    // special case for search address - return viewport around location
    return m_viewport[vID];
  }

  ASSERT(m_viewport[CURRENT_V].IsValid(), ());
  return m_viewport[CURRENT_V];
}

string DebugPrint(Query::ViewportID viewportId)
{
  switch (viewportId)
  {
    case Query::DEFAULT_V:
      return "Default";
    case Query::CURRENT_V:
      return "Current";
    case Query::LOCALITY_V:
      return "Locality";
    case Query::COUNT_V:
      return "Count";
  }
  ASSERT(false, ("Unknown viewportId"));
  return "Unknown";
}
}  // namespace search
