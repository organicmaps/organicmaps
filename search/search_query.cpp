#include "search_query.hpp"

#include "feature_offset_match.hpp"
#include "geometry_utils.hpp"
#include "indexed_value.hpp"
#include "latlon_match.hpp"
#include "search_common.hpp"
#include "search_query_params.hpp"
#include "search_string_intersection.hpp"

#include "storage/country_info_getter.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"
#include "indexer/feature_covering.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/index.hpp"
#include "indexer/scales.hpp"
#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "platform/preferred_languages.hpp"

#include "coding/multilang_utf8_string.hpp"
#include "coding/reader_wrapper.hpp"

#include "base/logging.hpp"
#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/function.hpp"

namespace search
{

namespace
{
using TCompareFunction1 = function<bool(impl::PreResult1 const &, impl::PreResult1 const &)>;
using TCompareFunction2 = function<bool(impl::PreResult2 const &, impl::PreResult2 const &)>;

TCompareFunction1 g_arrCompare1[] = {
    &impl::PreResult1::LessDistance, &impl::PreResult1::LessRank,
};

TCompareFunction2 g_arrCompare2[] = {
    &impl::PreResult2::LessDistance, &impl::PreResult2::LessRank,
};

  /// This indexes should match the initialization routine below.
  int g_arrLang1[] = { 0, 1, 2, 2, 3 };
  int g_arrLang2[] = { 0, 0, 0, 1, 0 };
  enum LangIndexT { LANG_CURRENT = 0,
                    LANG_INPUT,
                    LANG_INTERNATIONAL,
                    LANG_EN,
                    LANG_DEFAULT,
                    LANG_COUNT };

  pair<int, int> GetLangIndex(int id)
  {
    ASSERT_LESS ( id, LANG_COUNT, () );
    return make_pair(g_arrLang1[id], g_arrLang2[id]);
  }

  // Maximum result candidates count for each viewport/criteria.
  size_t const kPreResultsCount = 200;
}

Query::Query(Index & index, CategoriesHolder const & categories, vector<Suggest> const & suggests,
             storage::CountryInfoGetter const & infoGetter)
  : m_index(index)
  , m_categories(categories)
  , m_suggests(suggests)
  , m_infoGetter(infoGetter)
#ifdef HOUSE_SEARCH_TEST
  , m_houseDetector(&index)
#endif
#ifdef FIND_LOCALITY_TEST
  , m_locality(&index)
#endif
  , m_worldSearch(true)
{
  // m_viewport is initialized as empty rects

  // Results queue's initialization.
  static_assert(kQueuesCount == ARRAY_SIZE(g_arrCompare1), "");
  static_assert(kQueuesCount == ARRAY_SIZE(g_arrCompare2), "");

  for (size_t i = 0; i < kQueuesCount; ++i)
  {
    m_results[i] = TQueue(kPreResultsCount, TQueueCompare(g_arrCompare1[i]));
    m_results[i].reserve(kPreResultsCount);
  }

  // Initialize keywords scorer.
  // Note! This order should match the indexes arrays above.
  vector<vector<int8_t> > langPriorities(4);
  langPriorities[0].push_back(-1);   // future current lang
  langPriorities[1].push_back(-1);   // future input lang
  langPriorities[2].push_back(StringUtf8Multilang::GetLangIndex("int_name"));
  langPriorities[2].push_back(StringUtf8Multilang::GetLangIndex("en"));
  langPriorities[3].push_back(StringUtf8Multilang::GetLangIndex("default"));
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
    UpdateViewportOffsets(mwmsInfo, viewport, m_offsetsInViewport[idx]);

#ifdef FIND_LOCALITY_TEST
    m_locality.SetViewportByIndex(viewport, idx);
#endif
  }
  else
  {
    ClearCache(idx);

#ifdef FIND_LOCALITY_TEST
    m_locality.ClearCache(idx);
#endif
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

  m_houseDetector.ClearCaches();

  m_locality.ClearCacheAll();
}

void Query::ClearCache(size_t ind)
{
  // clear cache and free memory
  TOffsetsVector emptyV;
  emptyV.swap(m_offsetsInViewport[ind]);

  m_viewport[ind].MakeEmpty();
}

void Query::UpdateViewportOffsets(TMWMVector const & mwmsInfo, m2::RectD const & rect,
                                  TOffsetsVector & offsets)
{
  offsets.clear();

  int const queryScale = GetQueryIndexScale(rect);
  covering::CoveringGetter cov(rect, covering::ViewportWithLowLevels);

  for (shared_ptr<MwmInfo> const & info : mwmsInfo)
  {
    // Search only mwms that intersect with viewport (world always does).
    if (rect.IsIntersect(info->m_limitRect))
    {
      MwmSet::MwmId mwmId(info);
      Index::MwmHandle const mwmHandle = m_index.GetMwmHandleById(mwmId);
      if (MwmValue const * pMwm = mwmHandle.GetValue<MwmValue>())
      {
        TFHeader const & header = pMwm->GetHeader();
        if (header.GetType() == TFHeader::country)
        {
          pair<int, int> const scaleR = header.GetScaleRange();
          int const scale = min(max(queryScale, scaleR.first), scaleR.second);

          covering::IntervalsT const & interval = cov.Get(header.GetLastScale());

          ScaleIndex<ModelReaderPtr> index(pMwm->m_cont.GetReader(INDEX_FILE_TAG),
                                           pMwm->m_factory);

          for (size_t i = 0; i < interval.size(); ++i)
          {
            auto collectFn = MakeBackInsertFunctor(offsets[mwmId]);
            index.ForEachInIntervalAndScale(collectFn, interval[i].first, interval[i].second, scale);
          }

          sort(offsets[mwmId].begin(), offsets[mwmId].end());
        }
      }
    }
  }

#ifdef DEBUG
  size_t offsetsCached = 0;
  for (shared_ptr<MwmInfo> const & info : mwmsInfo)
  {
    MwmSet::MwmId mwmId(info);
    auto const it = offsets.find(mwmId);
    if (it != offsets.end())
      offsetsCached += it->second.size();
  }

  LOG(LDEBUG, ("For search in viewport cached mwms:", mwmsInfo.size(),
               "offsets:", offsetsCached));
#endif
}

void Query::Init(bool viewportSearch)
{
  Reset();

  m_tokens.clear();
  m_prefix.clear();

#ifdef HOUSE_SEARCH_TEST
  m_house.clear();
  m_streetID.clear();
#endif

  ClearQueues();

  if (viewportSearch)
  {
    // Special case to change comparator in viewport search
    // (more uniform results distribution on the map).

    m_queuesCount = 1;
    m_results[0] =
        TQueue(kPreResultsCount, TQueueCompare(&impl::PreResult1::LessPointsForViewport));
  }
  else
  {
    m_queuesCount = kQueuesCount;
    m_results[DISTANCE_TO_PIVOT] =
        TQueue(kPreResultsCount, TQueueCompare(g_arrCompare1[DISTANCE_TO_PIVOT]));
  }
}

void Query::ClearQueues()
{
  for (size_t i = 0; i < kQueuesCount; ++i)
    m_results[i].clear();
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
  m_query = &query;

  /// @todo Why Init is separated with SetQuery?
  ASSERT(m_tokens.empty(), ());
  ASSERT(m_prefix.empty(), ());
  ASSERT(m_house.empty(), ());

  // Split input query by tokens with possible last prefix.
  search::Delimiters delims;
  SplitUniString(NormalizeAndSimplifyString(query), MakeBackInsertFunctor(m_tokens), delims);

  bool checkPrefix = true;

  // Find most suitable token for house number.
#ifdef HOUSE_SEARCH_TEST
  int houseInd = static_cast<int>(m_tokens.size()) - 1;
  while (houseInd >= 0)
  {
    if (feature::IsHouseNumberDeepCheck(m_tokens[houseInd]))
    {
      if (m_tokens.size() > 1)
      {
        m_house.swap(m_tokens[houseInd]);
        m_tokens[houseInd].swap(m_tokens.back());
        m_tokens.pop_back();
      }
      break;
    }
    --houseInd;
  }

  // do check for prefix if last token is not a house number
  checkPrefix = m_house.empty() || houseInd < m_tokens.size();
#endif

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

void Query::SearchCoordinates(string const & query, Results & res) const
{
  double lat, lon;
  if (MatchLatLonDegree(query, lat, lon))
  {
    ASSERT_EQUAL(res.GetCount(), 0, ());
    res.AddResultNoChecks(MakeResult(impl::PreResult2(lat, lon)));
  }
}

void Query::Search(Results & res, size_t resCount)
{
  if (IsCancelled())
    return;

  if (m_tokens.empty())
    SuggestStrings(res);

  if (IsCancelled())
    return;
  SearchAddress(res);

  if (IsCancelled())
    return;
  SearchFeatures();

  if (IsCancelled())
    return;
  FlushResults(res, false, resCount);
}

namespace
{
  /// @name Functors to convert pointers to referencies.
  /// Pass them to stl algorithms.
  //@{
template <class TFunctor>
class ProxyFunctor1
  {
    TFunctor m_fn;

  public:
    template <class T>
    explicit ProxyFunctor1(T const & p) : m_fn(*p) {}

    template <class T>
    bool operator()(T const & p) { return m_fn(*p); }
  };

  template <class TFunctor>
  class ProxyFunctor2
  {
    TFunctor m_fn;

  public:
    template <class T>
    bool operator()(T const & p1, T const & p2)
    {
      return m_fn(*p1, *p2);
    }
  };
  //@}

  class IndexedValue : public search::IndexedValueBase<Query::kQueuesCount>
  {
    using ValueT = impl::PreResult2;

    /// @todo Do not use shared_ptr for optimization issues.
    /// Need to rewrite std::unique algorithm.
    shared_ptr<ValueT> m_val;

  public:
    explicit IndexedValue(ValueT * v) : m_val(v) {}

    ValueT const & operator*() const { return *m_val; }
    ValueT const * operator->() const { return m_val.get(); }

    string DebugPrint() const
    {
      string index;
      for (size_t i = 0; i < SIZE; ++i)
        index = index + " " + strings::to_string(m_ind[i]);

      return impl::DebugPrint(*m_val) + "; Index:" + index;
    }
  };

  inline string DebugPrint(IndexedValue const & t)
  {
    return t.DebugPrint();
  }

  struct CompFactory2
  {
    struct CompT
    {
      TCompareFunction2 m_fn;
      explicit CompT(TCompareFunction2 fn) : m_fn(fn) {}
      template <class T>
      bool operator()(T const & r1, T const & r2) const
      {
        return m_fn(*r1, *r2);
      }
    };

    static size_t const SIZE = 2;

    CompT Get(size_t i) { return CompT(g_arrCompare2[i]); }
  };

  struct LessFeatureID
  {
    using ValueT = impl::PreResult1;
    bool operator() (ValueT const & r1, ValueT const & r2) const
    {
      return (r1.GetID() < r2.GetID());
    }
  };

  class EqualFeatureID
  {
    using ValueT = impl::PreResult1;
    ValueT const & m_val;
  public:
    EqualFeatureID(ValueT const & v) : m_val(v) {}
    bool operator() (ValueT const & r) const
    {
      return (m_val.GetID() == r.GetID());
    }
  };

  template <class T>
  void RemoveDuplicatingLinear(vector<T> & indV)
  {
    sort(indV.begin(), indV.end(), ProxyFunctor2<impl::PreResult2::LessLinearTypesF>());
    indV.erase(unique(indV.begin(), indV.end(),
                      ProxyFunctor2<impl::PreResult2::EqualLinearTypesF>()),
               indV.end());
  }

  template <class T>
  bool IsResultExists(impl::PreResult2 const * p, vector<T> const & indV)
  {
    // do not insert duplicating results
    return (indV.end() != find_if(indV.begin(), indV.end(),
                                  ProxyFunctor1<impl::PreResult2::StrictEqualF>(p)));
  }
}

namespace impl
{
  class PreResult2Maker
  {
    Query & m_query;

    unique_ptr<Index::FeaturesLoaderGuard> m_pFV;

    // For the best performance, incoming id's should be sorted by id.first (mwm file id).
    void LoadFeature(FeatureID const & id, FeatureType & f, string & name, string & country)
    {
      if (m_pFV.get() == 0 || m_pFV->GetId() != id.m_mwmId)
        m_pFV.reset(new Index::FeaturesLoaderGuard(m_query.m_index, id.m_mwmId));

      m_pFV->GetFeatureByIndex(id.m_index, f);
      f.SetID(id);

      m_query.GetBestMatchName(f, name);

      // country (region) name is a file name if feature isn't from World.mwm
      if (m_pFV->IsWorld())
        country.clear();
      else
        country = m_pFV->GetCountryFileName();
    }

  public:
    PreResult2Maker(Query & q) : m_query(q)
    {
    }

    impl::PreResult2 * operator() (impl::PreResult1 const & res)
    {
      FeatureType feature;
      string name, country;
      LoadFeature(res.GetID(), feature, name, country);

      Query::ViewportID const viewportID = static_cast<Query::ViewportID>(res.GetViewportID());
      impl::PreResult2 * res2 = new impl::PreResult2(feature, &res,
                                                     m_query.GetPosition(viewportID),
                                                     name, country);

      /// @todo: add exluding of states (without USA states), continents
      using namespace ftypes;
      Type const tp = IsLocalityChecker::Instance().GetType(res2->m_types);
      switch (tp)
      {
      case COUNTRY:
        res2->m_rank /= 1.5;
        break;
      case CITY:
      case TOWN:
        if (m_query.GetViewport(Query::CURRENT_V).IsPointInside(res2->GetCenter()))
          res2->m_rank *= 2;
        else
        {
          storage::CountryInfo ci;
          res2->m_region.GetRegion(m_query.m_infoGetter, ci);
          if (ci.IsNotEmpty() && ci.m_name == m_query.GetPivotRegion())
            res2->m_rank *= 1.7;
        }
        break;
      case VILLAGE:
        res2->m_rank /= 1.5;
        break;
      default:
        break;
      }

      return res2;
    }

    impl::PreResult2 * operator() (FeatureID const & id)
    {
      FeatureType feature;
      string name, country;
      LoadFeature(id, feature, name, country);

      if (!name.empty() && !country.empty())
        return new impl::PreResult2(feature, 0, m_query.GetPosition(), name, country);
      else
        return 0;
    }
  };

  class HouseCompFactory
  {
    Query const & m_query;

    bool LessDistance(HouseResult const & r1, HouseResult const & r2) const
    {
      return (PointDistance(m_query.m_pivot, r1.GetOrg()) <
              PointDistance(m_query.m_pivot, r2.GetOrg()));
    }

  public:
    HouseCompFactory(Query const & q) : m_query(q) {}

    struct CompT
    {
      HouseCompFactory const * m_parent;

      CompT(HouseCompFactory const * parent) : m_parent(parent) {}
      bool operator() (HouseResult const & r1, HouseResult const & r2) const
      {
        return m_parent->LessDistance(r1, r2);
      }
    };

    static size_t const SIZE = 1;

    CompT Get(size_t) { return CompT(this); }
  };
}

template <class T>
void Query::MakePreResult2(vector<T> & cont, vector<FeatureID> & streets)
{
  // make unique set of PreResult1
  using TPreResultSet = set<impl::PreResult1, LessFeatureID>;
  TPreResultSet theSet;

  for (size_t i = 0; i < m_queuesCount; ++i)
  {
    theSet.insert(m_results[i].begin(), m_results[i].end());
    m_results[i].clear();
  }

  // make PreResult2 vector
  impl::PreResult2Maker maker(*this);
  for (auto const & r : theSet)
  {
    impl::PreResult2 * p = maker(r);
    if (p == 0)
      continue;

    if (p->IsStreet())
      streets.push_back(p->GetID());

    if (IsResultExists(p, cont))
      delete p;
    else
      cont.push_back(IndexedValue(p));
  }
}

void Query::FlushHouses(Results & res, bool allMWMs, vector<FeatureID> const & streets)
{
  if (!m_house.empty() && !streets.empty())
  {
    if (m_houseDetector.LoadStreets(streets) > 0)
      m_houseDetector.MergeStreets();

    m_houseDetector.ReadAllHouses();

    vector<HouseResult> houses;
    m_houseDetector.GetHouseForName(strings::ToUtf8(m_house), houses);

    SortByIndexedValue(houses, impl::HouseCompFactory(*this));

    // Limit address results when searching in first pass (position, viewport, locality).
    size_t count = houses.size();
    if (!allMWMs)
      count = min(count, size_t(5));

    for (size_t i = 0; i < count; ++i)
    {
      House const * h = houses[i].m_house;
      res.AddResult(MakeResult(impl::PreResult2(h->GetPosition(),
                                               h->GetNumber() + ", " + houses[i].m_street->GetName(),
                                               ftypes::IsBuildingChecker::Instance().GetMainType())));
    }
  }
}

void Query::FlushResults(Results & res, bool allMWMs, size_t resCount)
{
  vector<IndexedValue> indV;
  vector<FeatureID> streets;

  MakePreResult2(indV, streets);

  if (indV.empty())
    return;

  RemoveDuplicatingLinear(indV);

  SortByIndexedValue(indV, CompFactory2());

  // Do not process suggestions in additional search.
  if (!allMWMs || res.GetCount() == 0)
    ProcessSuggestions(indV, res);

#ifdef HOUSE_SEARCH_TEST
  FlushHouses(res, allMWMs, streets);
#endif

  // emit feature results
  size_t count = res.GetCount();
  for (size_t i = 0; i < indV.size() && count < resCount; ++i)
  {
    if (IsCancelled())
      break;

    LOG(LDEBUG, (indV[i]));

    if (res.AddResult(MakeResult(*(indV[i]))))
      ++count;
  }
}

void Query::SearchViewportPoints(Results & res)
{
  if (IsCancelled())
    return;
  SearchAddress(res);

  if (IsCancelled())
    return;
  SearchFeatures();

  vector<IndexedValue> indV;
  vector<FeatureID> streets;

  MakePreResult2(indV, streets);

  if (indV.empty())
    return;

  RemoveDuplicatingLinear(indV);

#ifdef HOUSE_SEARCH_TEST
  FlushHouses(res, false, streets);
#endif

  for (size_t i = 0; i < indV.size(); ++i)
  {
    if (IsCancelled())
      break;

    res.AddResultNoChecks(
          indV[i]->GeneratePointResult(&m_categories, &m_prefferedTypes, m_currentLocaleCode));
  }
}

int Query::GetQueryIndexScale(m2::RectD const & viewport) const
{
  return search::GetQueryIndexScale(viewport);
}

ftypes::Type Query::GetLocalityIndex(feature::TypesHolder const & types) const
{
  using namespace ftypes;

  // Inner logic of SearchAddress expects COUNTRY, STATE and CITY only.
  Type const type = IsLocalityChecker::Instance().GetType(types);
  switch (type)
  {
  case TOWN:
    return CITY;
  case VILLAGE:
    return NONE;
  default:
    return type;
  }
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
  // Split result's name on tokens.
  search::Delimiters delims;
  vector<strings::UniString> vName;
  SplitUniString(NormalizeAndSimplifyString(name), MakeBackInsertFunctor(vName), delims);

  // Find tokens that already present in input query.
  vector<bool> tokensMatched(vName.size());
  bool prefixMatched = false;
  for (size_t i = 0; i < vName.size(); ++i)
  {
    if (find(m_tokens.begin(), m_tokens.end(), vName[i]) != m_tokens.end())
      tokensMatched[i] = true;
    else
      if (vName[i].size() >= m_prefix.size() &&
          StartsWith(vName[i].begin(), vName[i].end(), m_prefix.begin(), m_prefix.end()))
      {
        prefixMatched = true;
      }
  }

  // Name doesn't match prefix - do nothing.
  if (!prefixMatched)
    return;

  RemoveStringPrefix(*m_query, suggest);

  // Append unmatched result's tokens to the suggestion.
  for (size_t i = 0; i < vName.size(); ++i)
    if (!tokensMatched[i])
    {
      suggest += strings::ToUtf8(vName[i]);
      suggest += " ";
    }
}

template <class T>
void Query::ProcessSuggestions(vector<T> & vec, Results & res) const
{
  if (m_prefix.empty())
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

void Query::AddResultFromTrie(TTrieValue const & val, MwmSet::MwmId const & mwmID,
                              ViewportID vID /*= DEFAULT_V*/)
{
  /// If we are in viewport search mode, check actual "point-in-viewport" criteria.
  /// @todo Actually, this checks are more-like hack, but not a suitable place to do ...
  if (m_queuesCount == 1 && vID == CURRENT_V && !m_viewport[CURRENT_V].IsPointInside(val.m_pt))
    return;

  impl::PreResult1 res(FeatureID(mwmID, val.m_featureId), val.m_rank,
                       val.m_pt, GetPosition(vID), vID);

  for (size_t i = 0; i < m_queuesCount; ++i)
  {
    // here can be the duplicates because of different language match (for suggest token)
    if (m_results[i].end() == find_if(m_results[i].begin(), m_results[i].end(), EqualFeatureID(res)))
      m_results[i].push(res);
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

  bool operator()(signed char lang, string const & name)
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

}  // namespace search::impl

void Query::GetBestMatchName(FeatureType const & f, string & name) const
{
  impl::BestNameFinder bestNameFinder(name, m_keywordsScorer);
  (void)f.ForEachNameRef(bestNameFinder);
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
  Result res = r.GenerateFinalResult(m_infoGetter, &m_categories,
                                     &m_prefferedTypes, m_currentLocaleCode);
  MakeResultHighlight(res);

#ifdef FIND_LOCALITY_TEST
  if (ftypes::IsLocalityChecker::Instance().GetType(r.GetTypes()) == ftypes::NONE)
  {
    string city;
    m_locality.GetLocalityInViewport(res.GetFeatureCenter(), city);
    res.AppendCity(city);
  }
#endif

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

namespace impl
{
  struct Locality
  {
    string m_name, m_enName;        ///< native name and english name of locality
    Query::TTrieValue m_value;
    vector<size_t> m_matchedTokens; ///< indexes of matched tokens for locality

    ftypes::Type m_type;
    double m_radius;

    Locality() : m_type(ftypes::NONE) {}
    Locality(Query::TTrieValue const & val, ftypes::Type type)
      : m_value(val), m_type(type), m_radius(0)
    {
    }

    bool IsValid() const
    {
      if (m_type != ftypes::NONE)
      {
        ASSERT ( !m_matchedTokens.empty(), () );
        return true;
      }
      else
        return false;
    }

    void Swap(Locality & rhs)
    {
      m_name.swap(rhs.m_name);
      m_enName.swap(rhs.m_enName);
      m_matchedTokens.swap(rhs.m_matchedTokens);

      using std::swap;
      swap(m_value, rhs.m_value);
      swap(m_type, rhs.m_type);
      swap(m_radius, rhs.m_radius);
    }

    bool operator< (Locality const & rhs) const
    {
      if (m_type != rhs.m_type)
        return (m_type < rhs.m_type);

      if (m_matchedTokens.size() != rhs.m_matchedTokens.size())
        return (m_matchedTokens.size() < rhs.m_matchedTokens.size());

      return (m_value.m_rank < rhs.m_value.m_rank);
    }

  private:
    class DoCount
    {
      size_t & m_count;
    public:
      DoCount(size_t & count) : m_count(count) { m_count = 0; }

      template <class T>
      void operator()(T const &) { ++m_count; }
    };

    bool IsFullNameMatched() const
    {
      size_t count;
      SplitUniString(NormalizeAndSimplifyString(m_name), DoCount(count), search::Delimiters());
      return (count <= m_matchedTokens.size());
    }

    using TString = strings::UniString;
    using TTokensArray = buffer_vector<TString, 32>;

    size_t GetSynonymTokenLength(TTokensArray const & tokens, TString const & prefix) const
    {
      // check only one token as a synonym
      if (m_matchedTokens.size() == 1)
      {
        size_t const index = m_matchedTokens[0];
        if (index < tokens.size())
          return tokens[index].size();
        else
        {
          ASSERT_EQUAL ( index, tokens.size(), () );
          ASSERT ( !prefix.empty(), () );
          return prefix.size();
        }
      }

      return size_t(-1);
    }

  public:
    /// Check that locality is suitable for source input tokens.
    bool IsSuitable(TTokensArray const & tokens, TString const & prefix) const
    {
      bool const isMatched = IsFullNameMatched();

      // Do filtering of possible localities.
      using namespace ftypes;

      switch (m_type)
      {
      case STATE:   // we process USA, Canada states only for now
        // USA states has 2-symbol synonyms
        return (isMatched || GetSynonymTokenLength(tokens, prefix) == 2);

      case COUNTRY:
        // USA has synonyms: "US" or "USA"
        return (isMatched ||
                (m_enName == "usa" && GetSynonymTokenLength(tokens, prefix) <= 3) ||
                (m_enName == "uk" && GetSynonymTokenLength(tokens, prefix) == 2));

      case CITY:
        // need full name match for cities
        return isMatched;

      default:
        ASSERT ( false, () );
        return false;
      }
    }
  };

  void swap(Locality & r1, Locality & r2) { r1.Swap(r2); }

  string DebugPrint(Locality const & l)
  {
    stringstream ss;
    ss << "{ Locality: " <<
          "Name = " + l.m_name <<
          "; Name English = " << l.m_enName <<
          "; Rank = " << int(l.m_value.m_rank) <<
          "; Matched: " << l.m_matchedTokens.size() << " }";
    return ss.str();
  }

  struct Region
  {
    vector<size_t> m_ids;
    vector<size_t> m_matchedTokens;
    string m_enName;

    bool operator<(Region const & rhs) const
    {
      return (m_matchedTokens.size() < rhs.m_matchedTokens.size());
    }

    bool IsValid() const
    {
      if (!m_ids.empty())
      {
        ASSERT ( !m_matchedTokens.empty(), () );
        ASSERT ( !m_enName.empty(), () );
        return true;
      }
      else
        return false;
    }

    void Swap(Region & rhs)
    {
      m_ids.swap(rhs.m_ids);
      m_matchedTokens.swap(rhs.m_matchedTokens);
      m_enName.swap(rhs.m_enName);
    }
  };

  string DebugPrint(Region const & r)
  {
    string res("Region: ");
    res += "Name English: " + r.m_enName;
    res += "; Matched: " + ::DebugPrint(r.m_matchedTokens.size());
    return res;
  }
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
      impl::Locality city;
      impl::Region region;
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

          m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(
                city.m_value.m_pt, city.m_radius);
          SetViewportByIndex(mwmsInfo, rect, LOCALITY_V, false);

          /// @todo Hack - do not search for address in World.mwm; Do it better in future.
          bool const b = m_worldSearch;
          m_worldSearch = false;
          SearchFeatures(params, mwmsInfo, LOCALITY_V);
          m_worldSearch = b;
        }
        else
        {
          // Add found locality as a result if nothing left to match.
          AddResultFromTrie(city.m_value, mwmId);
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
          for (shared_ptr<MwmInfo> & info : mwmsInfo)
          {
            Index::MwmHandle const handle = m_index.GetMwmHandleById(info);
            if (handle.IsAlive() &&
                m_infoGetter.IsBelongToRegions(handle.GetValue<MwmValue>()->GetCountryFileName(),
                                               region.m_ids))
            {
              SearchInMWM(handle, params);
            }
          }
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
    size_t m_index;         ///< index of processing token

    Locality * PushLocality(Locality const & l)
    {
      ASSERT ( 0 <= l.m_type && l.m_type <= ARRAY_SIZE(m_localities), (l.m_type) );
      m_localities[l.m_type].push_back(l);
      return &(m_localities[l.m_type].back());
    }

    int8_t m_lang;
    int8_t m_arrEn[3];

    /// Tanslates country full english name to mwm file name prefix
    /// (need when matching correspondent mwm file in CountryInfoGetter::GetMatchedRegions).
    //@{
    static bool FeatureName2FileNamePrefix(string & name, char const * prefix,
                                    char const * arr[], size_t n)
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

          char const * arrUSA[] = { "united", "states", "america" };
          char const * arrUK[] = { "united", "kingdom" };

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

    bool IsBelong(Locality const & loc, Region const & r) const
    {
      // check that locality and region are produced from different tokens
      vector<size_t> dummy;
      set_intersection(loc.m_matchedTokens.begin(), loc.m_matchedTokens.end(),
                       r.m_matchedTokens.begin(), r.m_matchedTokens.end(),
                       back_inserter(dummy));

      if (dummy.empty())
      {
        // check that locality belong to region
        return m_query.m_infoGetter.IsBelongToRegions(loc.m_value.m_pt, r.m_ids);
      }

      return false;
    }

    class EqualID
    {
      uint32_t m_id;
    public:
      EqualID(uint32_t id) : m_id(id) {}
      bool operator() (Locality const & l) const { return (l.m_value.m_featureId == m_id); }
    };

  public:
    DoFindLocality(Query & q, MwmValue const * pMwm, int8_t lang)
      : m_query(q), m_vector(pMwm->m_cont, pMwm->GetHeader(), pMwm->m_table), m_lang(lang)
    {
      m_arrEn[0] = q.GetLanguage(LANG_EN);
      m_arrEn[1] = q.GetLanguage(LANG_INTERNATIONAL);
      m_arrEn[2] = q.GetLanguage(LANG_DEFAULT);
    }

    void Resize(size_t) {}

    void SwitchTo(size_t ind) { m_index = ind; }

    void operator() (Query::TTrieValue const & v)
    {
      if (m_query.IsCancelled())
        throw Query::CancelException();

      // find locality in current results
      for (size_t i = 0; i < 3; ++i)
      {
        auto it = find_if(m_localities[i].begin(), m_localities[i].end(), EqualID(v.m_featureId));
        if (it != m_localities[i].end())
        {
          it->m_matchedTokens.push_back(m_index);
          return;
        }
      }

      // load feature
      FeatureType f;
      m_vector.GetByIndex(v.m_featureId, f);

      using namespace ftypes;

      // check, if feature is locality
      Type const index = m_query.GetLocalityIndex(feature::TypesHolder(f));
      if (index != NONE)
      {
        Locality * loc = PushLocality(Locality(v, index));
        if (loc)
        {
          loc->m_radius = GetRadiusByPopulation(GetPopulation(f));
          // m_lang name should exist if we matched feature in search index for this language.
          VERIFY(f.GetName(m_lang, loc->m_name), ());

          loc->m_matchedTokens.push_back(m_index);

          AssignEnglishName(f, *loc);
        }
      }
    }

    void SortLocalities()
    {
      for (int i = ftypes::COUNTRY; i <= ftypes::CITY; ++i)
        sort(m_localities[i].begin(), m_localities[i].end());
    }

    void GetRegions(vector<Region> & regions) const
    {
      //LOG(LDEBUG, ("Countries before processing = ", m_localities[ftypes::COUNTRY]));
      //LOG(LDEBUG, ("States before processing = ", m_localities[ftypes::STATE]));

      AddRegions(ftypes::STATE, regions);
      AddRegions(ftypes::COUNTRY, regions);

      //LOG(LDEBUG, ("Regions after processing = ", regions));
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
          if (IsBelong(*i, regions[j]))
            belongs.push_back(&regions[j]);
        }

        for (size_t j = 0; j < belongs.size(); ++j)
        {
          // splice locality info with region info
          i->m_matchedTokens.insert(i->m_matchedTokens.end(),
                                    belongs[j]->m_matchedTokens.begin(),
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
}

void Query::SearchLocality(MwmValue const * pMwm, impl::Locality & res1, impl::Region & res2)
{
  SearchQueryParams params;
  InitParams(true /* localitySearch */, params);

  serial::CodingParams cp(trie::GetCodingParams(pMwm->GetHeader().GetDefCodingParams()));

  ModelReaderPtr searchReader = pMwm->m_cont.GetReader(SEARCH_INDEX_FILE_TAG);

  unique_ptr<trie::DefaultIterator> const trieRoot(
      trie::ReadTrie(SubReaderWrapper<Reader>(searchReader.GetPtr()), trie::ValueReader(cp),
                     trie::TEdgeValueReader()));

  ForEachLangPrefix(params, *trieRoot, [&](TrieRootPrefix & langRoot, int8_t lang)
  {
    impl::DoFindLocality doFind(*this, pMwm, lang);
    MatchTokensInTrie(params.m_tokens, langRoot, doFind);

    // Last token's prefix is used as a complete token here, to limit number of results.
    doFind.Resize(params.m_tokens.size() + 1);
    doFind.SwitchTo(params.m_tokens.size());
    MatchTokenInTrie(params.m_prefixTokens, langRoot, doFind);
    doFind.SortLocalities();

    // Get regions from STATE and COUNTRY localities
    vector<impl::Region> regions;
    doFind.GetRegions(regions);

    // Get best CITY locality.
    impl::Locality loc;
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
  });
}

void Query::SearchFeatures()
{
  TMWMVector mwmsInfo;
  m_index.GetMwmsInfo(mwmsInfo);

  SearchQueryParams params;
  InitParams(false /* localitySearch */, params);

  // do usual search in viewport and near me (without last rect)
  for (int i = 0; i < LOCALITY_V; ++i)
  {
    if (m_viewport[i].IsValid())
      SearchFeatures(params, mwmsInfo, static_cast<ViewportID>(i));
  }
}

namespace
{
  class FeaturesFilter
  {
    vector<uint32_t> const * m_offsets;

    my::Cancellable const & m_cancellable;
  public:
    FeaturesFilter(vector<uint32_t> const * offsets, my::Cancellable const & cancellable)
      : m_offsets(offsets), m_cancellable(cancellable)
    {
    }

    bool operator() (uint32_t offset) const
    {
      if (m_cancellable.IsCancelled())
      {
        //LOG(LINFO, ("Throw CancelException"));
        //dbg::ObjectTracker::PrintLeaks();
        throw Query::CancelException();
      }

      return (m_offsets == 0 ||
              binary_search(m_offsets->begin(), m_offsets->end(), offset));
    }
  };
}

void Query::SearchFeatures(SearchQueryParams const & params, TMWMVector const & mwmsInfo,
                           ViewportID vID)
{
  for (shared_ptr<MwmInfo> const & info : mwmsInfo)
  {
    // Search only mwms that intersect with viewport (world always does).
    if (m_viewport[vID].IsIntersect(info->m_limitRect))
      SearchInMWM(m_index.GetMwmHandleById(info), params, vID);
  }
}

void Query::SearchInMWM(Index::MwmHandle const & mwmHandle, SearchQueryParams const & params,
                        ViewportID viewportId /*= DEFAULT_V*/)
{
  MwmValue const * const value = mwmHandle.GetValue<MwmValue>();
  if (!value || !value->m_cont.IsExist(SEARCH_INDEX_FILE_TAG))
    return;

  TFHeader const & header = value->GetHeader();
  /// @todo do not process World.mwm here - do it in SearchLocality
  bool const isWorld = (header.GetType() == TFHeader::world);
  if (isWorld && !m_worldSearch)
    return;

  serial::CodingParams cp(trie::GetCodingParams(header.GetDefCodingParams()));
  ModelReaderPtr searchReader = value->m_cont.GetReader(SEARCH_INDEX_FILE_TAG);
  unique_ptr<trie::DefaultIterator> const trieRoot(
      trie::ReadTrie(SubReaderWrapper<Reader>(searchReader.GetPtr()), trie::ValueReader(cp),
                     trie::TEdgeValueReader()));

  MwmSet::MwmId const mwmId = mwmHandle.GetId();
  FeaturesFilter filter(viewportId == DEFAULT_V || isWorld ?
                          0 : &m_offsetsInViewport[viewportId][mwmId], *this);
  MatchFeaturesInTrie(params, *trieRoot, filter, [&](TTrieValue const & value)
  {
    AddResultFromTrie(value, mwmId, viewportId);
  });
}

void Query::SuggestStrings(Results & res)
{
  if (!m_prefix.empty())
  {
    int8_t arrLocales[3];
    int const localesCount = GetCategoryLocales(arrLocales);

    string prolog;
    RemoveStringPrefix(*m_query, prolog);

    for (int i = 0; i < localesCount; ++i)
      MatchForSuggestionsImpl(m_prefix, arrLocales[i], prolog, res);
  }
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

m2::PointD Query::GetPosition(ViewportID vID /*= DEFAULT_V*/) const
{
  switch (vID)
  {
  case LOCALITY_V:      // center of the founded locality
    return m_viewport[vID].Center();
  default:
    return m_pivot;
  }
}

void Query::SearchAdditional(Results & res, size_t resCount)
{
  ClearQueues();

  string const fileName = m_infoGetter.GetRegionFile(m_pivot);
  if (!fileName.empty())
  {
    LOG(LDEBUG, ("Additional MWM search: ", fileName));

    TMWMVector mwmsInfo;
    m_index.GetMwmsInfo(mwmsInfo);

    SearchQueryParams params;
    InitParams(false /* localitySearch */, params);

    for (shared_ptr<MwmInfo> const & info : mwmsInfo)
    {
      Index::MwmHandle const handle = m_index.GetMwmHandleById(info);
      if (handle.IsAlive() &&
          handle.GetValue<MwmValue>()->GetCountryFileName() == fileName)
      {
        SearchInMWM(handle, params);
      }
    }

#ifdef FIND_LOCALITY_TEST
    m2::RectD rect;
    for (auto const & r : m_results[0])
      rect.Add(r.GetCenter());

    // Hack with 90.0 is important for the countries divided by 180 meridian.
    if (rect.IsValid() && (rect.maxX() - rect.minX()) <= 90.0)
      m_locality.SetReservedViewportIfNeeded(rect);
#endif

    FlushResults(res, true, resCount);
  }
}

}  // namespace search
