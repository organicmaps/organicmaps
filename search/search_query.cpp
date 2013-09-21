#include "search_query.hpp"
#include "feature_offset_match.hpp"
#include "latlon_match.hpp"
#include "search_common.hpp"

#include "../storage/country_info.hpp"

#include "../indexer/feature_covering.hpp"
#include "../indexer/features_vector.hpp"
#include "../indexer/index.hpp"
#include "../indexer/scales.hpp"
#include "../indexer/search_delimiters.hpp"
#include "../indexer/search_string_utils.hpp"
#include "../indexer/categories_holder.hpp"
#include "../indexer/classificator.hpp"

#include "../coding/multilang_utf8_string.hpp"
#include "../coding/reader_wrapper.hpp"

#include "../base/logging.hpp"
#include "../base/string_utils.hpp"
#include "../base/stl_add.hpp"

#include "../std/algorithm.hpp"
#include "../std/array.hpp"
#include "../std/bind.hpp"


namespace search
{

namespace
{
  typedef bool (*CompareFunctionT1) (impl::PreResult1 const &, impl::PreResult1 const &);
  typedef bool (*CompareFunctionT2) (impl::PreResult2 const &, impl::PreResult2 const &);

  CompareFunctionT1 g_arrCompare1[] =
  {
    &impl::PreResult1::LessRank,
    &impl::PreResult1::LessViewportDistance,
    &impl::PreResult1::LessDistance
  };

  CompareFunctionT2 g_arrCompare2[] =
  {
    &impl::PreResult2::LessRank,
    &impl::PreResult2::LessViewportDistance,
    &impl::PreResult2::LessDistance
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
}


Query::Query(Index const * pIndex,
             CategoriesHolder const * pCategories,
             StringsToSuggestVectorT const * pStringsToSuggest,
             storage::CountryInfoGetter const * pInfoGetter,
             size_t resultsNeeded /*= 10*/)
  : m_pIndex(pIndex),
    m_pCategories(pCategories),
    m_pStringsToSuggest(pStringsToSuggest),
    m_pInfoGetter(pInfoGetter),
    m_worldSearch(true),
    m_position(empty_pos_value, empty_pos_value)
{
  // m_viewport is initialized as empty rects

  ASSERT ( m_pIndex, () );

  // Results queue's initialization.
  STATIC_ASSERT ( m_qCount == ARRAY_SIZE(g_arrCompare1) );
  STATIC_ASSERT ( m_qCount == ARRAY_SIZE(g_arrCompare2) );

  for (size_t i = 0; i < m_qCount; ++i)
  {
    m_results[i] = QueueT(2 * resultsNeeded, QueueCompareT(g_arrCompare1[i]));
    m_results[i].reserve(2 * resultsNeeded);
  }

  // Initialize keywords scorer.
  // Note! This order should match the indexes arrays above.
  vector<vector<int8_t> > langPriorities(4);
  langPriorities[0].push_back(0);   // future current lang
  langPriorities[1].push_back(0);   // future input lang
  langPriorities[2].push_back(StringUtf8Multilang::GetLangIndex("int_name"));
  langPriorities[2].push_back(StringUtf8Multilang::GetLangIndex("en"));
  langPriorities[3].push_back(StringUtf8Multilang::GetLangIndex("default"));
  m_keywordsScorer.SetLanguages(langPriorities);

  SetPreferredLanguage("en");
}

Query::~Query()
{
}

void Query::SetLanguage(int id, int8_t lang)
{
  m_keywordsScorer.SetLanguage(GetLangIndex(id), lang);
}

int8_t Query::GetLanguage(int id) const
{
  return m_keywordsScorer.GetLanguage(GetLangIndex(id));
}

namespace
{
  inline bool IsEqualMercator(m2::RectD const & r1, m2::RectD const & r2, double epsMeters)
  {
    double const eps = epsMeters * MercatorBounds::degreeInMetres;
    return m2::IsEqual(r1, r2, eps, eps);
  }
}

void Query::SetViewport(m2::RectD viewport[], size_t count)
{
  // use static_cast to avoid GCC linker dummy bug
  ASSERT_LESS ( count, static_cast<size_t>(RECTSCOUNT), () );

  m_cancel = false;

  MWMVectorT mwmInfo;
  m_pIndex->GetMwmInfo(mwmInfo);

  for (size_t i = 0; i < count; ++i)
    SetViewportByIndex(mwmInfo, viewport[i], i);
}

void Query::SetViewportByIndex(MWMVectorT const & mwmInfo, m2::RectD const & viewport, size_t idx)
{
  // use static_cast to avoid GCC linker dummy bug
  ASSERT_LESS ( idx, static_cast<size_t>(RECTSCOUNT), () );

  if (viewport.IsValid())
  {
    // Check if viewports are equal (10 meters).
    if (!m_viewport[idx].IsValid() || !IsEqualMercator(m_viewport[idx], viewport, 10.0))
    {
      m_viewport[idx] = viewport;
      UpdateViewportOffsets(mwmInfo, viewport, m_offsetsInViewport[idx]);
    }
  }
  else
    ClearCache(idx);
}

void Query::SetPreferredLanguage(string const & lang)
{
  int8_t const code = StringUtf8Multilang::GetLangIndex(lang);
  SetLanguage(LANG_CURRENT, code);

  // Default initialization.
  // If you want to reset input language, call SetInputLanguage before search.
  SetInputLanguage(code);
}

void Query::SetInputLanguage(int8_t lang)
{
  LOG(LDEBUG, ("New input language = ", lang));
  SetLanguage(LANG_INPUT, lang);
}

int8_t Query::GetPrefferedLanguage() const
{
  return GetLanguage(LANG_CURRENT);
}

void Query::ClearCaches()
{
  for (size_t i = 0; i < RECTSCOUNT; ++i)
    ClearCache(i);
}

void Query::ClearCache(size_t ind)
{
  // clear cache and free memory
  OffsetsVectorT emptyV;
  emptyV.swap(m_offsetsInViewport[ind]);

  m_viewport[ind].MakeEmpty();
}

void Query::UpdateViewportOffsets(MWMVectorT const & mwmInfo, m2::RectD const & rect,
                                  OffsetsVectorT & offsets)
{
  offsets.clear();
  offsets.resize(mwmInfo.size());

  int const viewScale = scales::GetScaleLevel(rect);
  covering::CoveringGetter cov(rect, 0);

  for (MwmSet::MwmId mwmId = 0; mwmId < mwmInfo.size(); ++mwmId)
  {
    // Search only mwms that intersect with viewport (world always does).
    if (rect.IsIntersect(mwmInfo[mwmId].m_limitRect))
    {
      Index::MwmLock mwmLock(*m_pIndex, mwmId);
      if (MwmValue * pMwm = mwmLock.GetValue())
      {
        FHeaderT const & header = pMwm->GetHeader();
        if (header.GetType() == FHeaderT::country)
        {
          pair<int, int> const scaleR = header.GetScaleRange();
          int const scale = min(max(viewScale + SCALE_SEARCH_DEPTH, scaleR.first), scaleR.second);

          covering::IntervalsT const & interval = cov.Get(header.GetLastScale());

          ScaleIndex<ModelReaderPtr> index(pMwm->m_cont.GetReader(INDEX_FILE_TAG),
                                           pMwm->m_factory);

          for (size_t i = 0; i < interval.size(); ++i)
          {
            index.ForEachInIntervalAndScale(MakeBackInsertFunctor(offsets[mwmId]),
                                            interval[i].first, interval[i].second,
                                            scale);
          }

          sort(offsets[mwmId].begin(), offsets[mwmId].end());
        }
      }
    }
  }

#ifdef DEBUG
  size_t offsetsCached = 0;
  for (MwmSet::MwmId mwmId = 0; mwmId < mwmInfo.size(); ++mwmId)
    offsetsCached += offsets[mwmId].size();

  LOG(LDEBUG, ("For search in viewport cached ",
              "mwms:", mwmInfo.size(),
              "offsets:", offsetsCached));
#endif
}

void Query::Init()
{
  m_cancel = false;

  m_tokens.clear();
  m_prefix.clear();

  ClearQueues();
}

void Query::ClearQueues()
{
  for (size_t i = 0; i < m_qCount; ++i)
    m_results[i].clear();
}

void Query::SetQuery(string const & query)
{
  // split input query by tokens and prefix
  search::Delimiters delims;
  SplitUniString(NormalizeAndSimplifyString(query), MakeBackInsertFunctor(m_tokens), delims);

  if (!m_tokens.empty() && !delims(strings::LastUniChar(query)))
  {
    m_prefix.swap(m_tokens.back());
    m_tokens.pop_back();
  }

  int const maxTokensCount = MAX_TOKENS-1;
  if (m_tokens.size() > maxTokensCount)
    m_tokens.resize(maxTokensCount);

  // assign tokens and prefix to scorer
  m_keywordsScorer.SetKeywords(m_tokens.data(), m_tokens.size(), m_prefix);
}

void Query::SearchCoordinates(string const & query, Results & res) const
{
  double lat, lon, latPrec, lonPrec;
  if (search::MatchLatLon(query, lat, lon, latPrec, lonPrec))
  {
    //double const precision = 5.0 * max(0.0001, min(latPrec, lonPrec));  // Min 55 meters
    res.AddResult(MakeResult(impl::PreResult2(GetViewport(), m_position, lat, lon)));
  }
}

void Query::Search(Results & res, bool searchAddress)
{
  ClearQueues();

  if (m_cancel) return;
  SuggestStrings(res);

  if (m_cancel) return;
  if (searchAddress)
    SearchAddress();

  if (m_cancel) return;
  SearchFeatures();

  if (m_cancel) return;
  FlushResults(res, &Results::AddResult);
}

namespace
{
  /// @name Functors to convert pointers to referencies.
  /// Pass them to stl algorithms.
  //@{
  template <class FunctorT> class ProxyFunctor1
  {
    FunctorT m_fn;
  public:
    template <class T> explicit ProxyFunctor1(T const & p) : m_fn(*p) {}
    template <class T> bool operator() (T const & p) { return m_fn(*p); }
  };

  template <class FunctorT> class ProxyFunctor2
  {
    FunctorT m_fn;
  public:
    template <class T> bool operator() (T const & p1, T const & p2)
    {
      return m_fn(*p1, *p2);
    }
  };
  //@}

  class IndexedValue
  {
  public:
     typedef impl::PreResult2 element_type;

  private:
    array<size_t, Query::m_qCount> m_ind;

    /// @todo Do not use shared_ptr for optimization issues.
    /// Need to rewrite std::unique algorithm.
    shared_ptr<element_type> m_val;

  public:
    explicit IndexedValue(element_type * v) : m_val(v)
    {
      for (size_t i = 0; i < m_ind.size(); ++i)
        m_ind[i] = numeric_limits<size_t>::max();
    }

    element_type const & operator*() const { return *m_val; }

    void SetIndex(size_t i, size_t v) { m_ind[i] = v; }

    void SortIndex()
    {
      sort(m_ind.begin(), m_ind.end());
    }

    string DebugPrint() const
    {
      string index;
      for (size_t i = 0; i < m_ind.size(); ++i)
        index = index + " " + strings::to_string(m_ind[i]);

      return impl::DebugPrint(*m_val) + "; Index:" + index;
    }

    bool operator < (IndexedValue const & r) const
    {
      for (size_t i = 0; i < m_ind.size(); ++i)
      {
        if (m_ind[i] != r.m_ind[i])
          return (m_ind[i] < r.m_ind[i]);
      }

      return false;
    }
  };

  inline string DebugPrint(IndexedValue const & t)
  {
    return t.DebugPrint();
  }

  struct LessByFeatureID
  {
    typedef impl::PreResult1 ValueT;
    bool operator() (ValueT const & r1, ValueT const & r2) const
    {
      return (r1.GetID() < r2.GetID());
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
  void AddPreResult2(impl::PreResult2 * p, vector<T> & indV)
  {
    if (p)
    {
      // do not insert duplicating results
      if (indV.end() == find_if(indV.begin(), indV.end(),
                                ProxyFunctor1<impl::PreResult2::StrictEqualF>(p)))
        indV.push_back(T(p));
      else
        delete p;
    }
  }
}

namespace impl
{
  class PreResult2Maker
  {
    Query & m_query;

    scoped_ptr<Index::FeaturesLoaderGuard> m_pFV;

    // For the best performance, incoming id's should be sorted by id.first (mwm file id).
    void LoadFeature(FeatureID const & id, FeatureType & f, string & name, string & country)
    {
      if (m_pFV.get() == 0 || m_pFV->GetID() != id.m_mwm)
        m_pFV.reset(new Index::FeaturesLoaderGuard(*m_query.m_pIndex, id.m_mwm));

      m_pFV->GetFeature(id.m_offset, f);
      f.SetID(id);

      m_query.GetBestMatchName(f, name);

      // country (region) name is a file name if feature isn't from World.mwm
      if (m_pFV->IsWorld())
        country.clear();
      else
        country = m_pFV->GetFileName();
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

      int8_t const viewportID = res.GetViewportID();
      return new impl::PreResult2(feature, &res,
                                  m_query.GetViewport(viewportID), m_query.GetPosition(viewportID),
                                  name, country);
    }

    impl::PreResult2 * operator() (FeatureID const & id)
    {
      FeatureType feature;
      string name, country;
      LoadFeature(id, feature, name, country);

      if (!name.empty() && !country.empty())
      {
        return new impl::PreResult2(feature, 0,
                                    m_query.GetViewport(), m_query.GetPosition(),
                                    name, country);
      }
      else
        return 0;
    }
  };
}

void Query::FlushResults(Results & res, void (Results::*pAddFn)(Result const &))
{
  vector<IndexedValue> indV;

  {
    // make unique set of PreResult1
    typedef set<impl::PreResult1, LessByFeatureID> PreResultSetT;
    PreResultSetT theSet;

    for (size_t i = 0; i < m_qCount; ++i)
    {
      theSet.insert(m_results[i].begin(), m_results[i].end());
      m_results[i].clear();
    }

    // make PreResult2 vector
    impl::PreResult2Maker maker(*this);
    for (PreResultSetT::const_iterator i = theSet.begin(); i != theSet.end(); ++i)
      AddPreResult2(maker(*i), indV);
  }

  if (indV.empty())
    return;

  RemoveDuplicatingLinear(indV);

  for (size_t i = 0; i < m_qCount; ++i)
  {
    CompareT<impl::PreResult2, RefPointer> comp(g_arrCompare2[i]);

    // sort by needed criteria
    sort(indV.begin(), indV.end(), comp);

    // assign ranks
    size_t rank = 0;
    for (size_t j = 0; j < indV.size(); ++j)
    {
      if (j > 0 && comp(indV[j-1], indV[j]))
        ++rank;

      indV[j].SetIndex(i, rank);
    }
  }

  // prepare combined criteria
  for_each(indV.begin(), indV.end(), bind(&IndexedValue::SortIndex, _1));

  // sort results according to combined criteria
  sort(indV.begin(), indV.end());

  // get preffered types to show in results
  set<uint32_t> prefferedTypes;
  if (m_pCategories)
  {
    for (size_t i = 0; i < m_tokens.size(); ++i)
      m_pCategories->ForEachTypeByName(m_tokens[i], MakeInsertFunctor(prefferedTypes));

    if (!m_prefix.empty())
      m_pCategories->ForEachTypeByName(m_prefix, MakeInsertFunctor(prefferedTypes));
  }

  // emit feature results
  for (size_t i = 0; i < indV.size(); ++i)
  {
    if (m_cancel) break;

    LOG(LDEBUG, (indV[i]));

    (res.*pAddFn)(MakeResult(*(indV[i]), &prefferedTypes));
  }
}

namespace
{
  class EqualFeature
  {
    typedef impl::PreResult1 ValueT;
    ValueT const & m_val;

  public:
    EqualFeature(ValueT const & v) : m_val(v) {}
    bool operator() (ValueT const & r) const
    {
      return (m_val.GetID() == r.GetID());
    }
  };
}

void Query::AddResultFromTrie(TrieValueT const & val, size_t mwmID, int8_t viewportID)
{
  impl::PreResult1 res(FeatureID(mwmID, val.m_featureId), val.m_rank, val.m_pt,
                       GetPosition(viewportID), GetViewport(viewportID), viewportID);

  for (size_t i = 0; i < m_qCount; ++i)
  {
    // here can be the duplicates because of different language match (for suggest token)
    if (m_results[i].end() == find_if(m_results[i].begin(), m_results[i].end(), EqualFeature(res)))
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

Result Query::MakeResult(impl::PreResult2 const & r, set<uint32_t> const * pPrefferedTypes/* = 0*/) const
{
  return r.GenerateFinalResult(m_pInfoGetter, m_pCategories, pPrefferedTypes, GetLanguage(LANG_CURRENT));
}

namespace impl
{

class FeatureLoader
{
  Query & m_query;
  size_t m_mwmID, m_count;
  int8_t m_viewportID;

public:
  FeatureLoader(Query & query, size_t mwmID, int viewportID)
    : m_query(query), m_mwmID(mwmID), m_count(0),
      m_viewportID(static_cast<int8_t>(viewportID))
  {
  }

  void operator() (Query::TrieValueT const & value)
  {
    ++m_count;
    m_query.AddResultFromTrie(value, m_mwmID, m_viewportID);
  }

  size_t GetCount() const { return m_count; }
  void Reset() { m_count = 0; }
};

}

namespace
{
  typedef vector<strings::UniString> TokensVectorT;

  class DoInsertTypeNames
  {
    Classificator const & m_c;
    TokensVectorT & m_tokens;
    bool m_supportOldFormat;

    static int GetOldTypeFromIndex(size_t index)
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

  public:
    DoInsertTypeNames(TokensVectorT & tokens, bool supportOldFormat)
      : m_c(classif()), m_tokens(tokens), m_supportOldFormat(supportOldFormat)
    {
    }
    void operator() (uint32_t t)
    {
      uint32_t const index = m_c.GetIndexForType(t);
      m_tokens.push_back(FeatureTypeToString(index));

      // v2-version MWM has raw classificator types in search index prefix, so
      // do the hack: add synonyms for old convention if needed.
      if (m_supportOldFormat)
      {
        int const type = GetOldTypeFromIndex(index);
        if (type >= 0)
        {
          ASSERT ( type == 70 || type > 4000, (type));
          m_tokens.push_back(FeatureTypeToString(static_cast<uint32_t>(type)));
        }
      }
    }
  };
}  // namespace search::impl

Query::Params::Params(Query const & q, bool isLocalities/* = false*/)
{
  if (!q.m_prefix.empty())
    m_prefixTokens.push_back(q.m_prefix);

  size_t const tokensCount = q.m_tokens.size();
  m_tokens.resize(tokensCount);

  // Add normal tokens.
  for (size_t i = 0; i < tokensCount; ++i)
    m_tokens[i].push_back(q.m_tokens[i]);

  // Add names of categories (and synonyms).
  if (q.m_pCategories && !isLocalities)
  {
    for (size_t i = 0; i < tokensCount; ++i)
      q.m_pCategories->ForEachTypeByName(q.m_tokens[i], DoInsertTypeNames(m_tokens[i], q.m_supportOldFormat));

    if (!q.m_prefix.empty())
      q.m_pCategories->ForEachTypeByName(q.m_prefix, DoInsertTypeNames(m_prefixTokens, q.m_supportOldFormat));
  }

  FillLanguages(q);
}

void Query::Params::EraseTokens(vector<size_t> & eraseInds)
{
  eraseInds.erase(unique(eraseInds.begin(), eraseInds.end()), eraseInds.end());

  // fill temporary vector
  vector<TokensVectorT> newTokens;

  size_t skipI = 0;
  size_t const count = m_tokens.size();
  size_t const eraseCount = eraseInds.size();
  for (size_t i = 0; i < count; ++i)
  {
    if (skipI < eraseCount && eraseInds[skipI] == i)
    {
      ++skipI;
    }
    else
    {
      newTokens.push_back(TokensVectorT());
      newTokens.back().swap(m_tokens[i]);
    }
  }

  // assign to m_tokens
  newTokens.swap(m_tokens);

  if (skipI < eraseCount)
  {
    // it means that we need to skip prefix tokens
    ASSERT_EQUAL ( skipI+1, eraseCount, (eraseInds) );
    ASSERT_EQUAL ( eraseInds[skipI], count, (eraseInds) );
    m_prefixTokens.clear();
  }
}

template <class ToDo> void Query::Params::ForEachToken(ToDo toDo)
{
  size_t const count = m_tokens.size();
  for (size_t i = 0; i < count; ++i)
  {
    ASSERT ( !m_tokens[i].empty(), () );
    ASSERT ( !m_tokens[i].front().empty(), () );
    toDo(m_tokens[i].front(), i);
  }

  if (!m_prefixTokens.empty())
  {
    ASSERT ( !m_prefixTokens.front().empty(), () );
    toDo(m_prefixTokens.front(), count);
  }
}

string DebugPrint(Query::Params const & p)
{
  return ("Query::Params: Tokens = " + DebugPrint(p.m_tokens) +
          "; Prefixes = " + DebugPrint(p.m_prefixTokens));
}

namespace
{
  bool IsNumber(strings::UniString const & s)
  {
    for (size_t i = 0; i < s.size(); ++i)
    {
      // android production ndk-r8d has bug. "еда" detected as a number.
      if (s[i] > 127 || !isdigit(s[i]))
        return false;
    }
    return true;
  }

  class DoStoreNumbers
  {
    vector<size_t> & m_vec;
  public:
    DoStoreNumbers(vector<size_t> & vec) : m_vec(vec) {}
    void operator() (Query::Params::StringT const & s, size_t i)
    {
      /// @todo Do smart filtering of house numbers and zipcodes.
      if (IsNumber(s))
        m_vec.push_back(i);
    }
  };

  class DoAddStreetSynonyms
  {
    Query::Params & m_params;

    Query::Params::TokensVectorT & GetTokens(size_t i)
    {
      size_t const count = m_params.m_tokens.size();
      if (i < count)
        return m_params.m_tokens[i];
      else
      {
        ASSERT_EQUAL ( i, count, () );
        return m_params.m_prefixTokens;
      }
    }

    void AddSynonym(size_t i, string const & sym)
    {
      GetTokens(i).push_back(strings::MakeUniString(sym));
    }

  public:
    DoAddStreetSynonyms(Query::Params & params) : m_params(params) {}

    void operator() (Query::Params::StringT const & s, size_t i)
    {
      if (s.size() <= 2)
      {
        string const ss = strings::ToUtf8(strings::MakeLowerCase(s));

        // All synonyms should be lowercase!
        if (ss == "n")
          AddSynonym(i, "north");
        else if (ss == "w")
          AddSynonym(i, "west");
        else if (ss == "s")
          AddSynonym(i, "south");
        else if (ss == "e")
          AddSynonym(i, "east");
        else if (ss == "nw")
          AddSynonym(i, "northwest");
        else if (ss == "ne")
          AddSynonym(i, "northeast");
        else if (ss == "sw")
          AddSynonym(i, "southwest");
        else if (ss == "se")
          AddSynonym(i, "southeast");
      }
    }
  };
}

void Query::Params::ProcessAddressTokens()
{
  // 1. Do simple stuff - erase all number tokens.
  // Assume that USA street name numbers are end with "st, nd, rd, th" suffixes.

  vector<size_t> toErase;
  ForEachToken(DoStoreNumbers(toErase));
  EraseTokens(toErase);

  // 2. Add synonyms for N, NE, NW, etc.
  ForEachToken(DoAddStreetSynonyms(*this));
}

void Query::Params::FillLanguages(Query const & q)
{
  for (int i = 0; i < LANG_COUNT; ++i)
    m_langs.insert(q.GetLanguage(i));
}

namespace impl
{
  struct Locality
  {
    string m_name, m_enName;        ///< native name and english name of locality
    Query::TrieValueT m_value;
    vector<size_t> m_matchedTokens; ///< indexes of matched tokens for locality

    /// Type of locality (do not change values - they have detalization order)
    /// COUNTRY < STATE < CITY
    enum Type { NONE = -1, COUNTRY = 0, STATE, CITY };
    Type m_type;

    Locality() : m_type(NONE) {}
    Locality(Query::TrieValueT const & val, Type type) : m_value(val), m_type(type) {}

    bool IsValid() const
    {
      if (m_type != NONE)
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
      swap(m_value, rhs.m_value);
      m_matchedTokens.swap(rhs.m_matchedTokens);
      swap(m_type, rhs.m_type);
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
      template <class T> void operator() (T const &) { ++m_count; }
    };

    bool IsFullNameMatched() const
    {
      size_t count;
      SplitUniString(NormalizeAndSimplifyString(m_name), DoCount(count), search::Delimiters());
      return (count <= m_matchedTokens.size());
    }

    typedef strings::UniString StringT;
    typedef buffer_vector<StringT, 32> TokensArrayT;

    size_t GetSynonymTokenLength(TokensArrayT const & tokens, StringT const & prefix) const
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
    bool IsSuitable(TokensArrayT const & tokens, StringT const & prefix) const
    {
      bool const isMatched = IsFullNameMatched();

      // Do filtering of possible localities.
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
    string res("Locality: ");
    res += "Name: " + l.m_name;
    res += "; Name English: " + l.m_enName;
    res += "; Rank: " + ::DebugPrint(l.m_value.m_rank);
    res += "; Matched: " + ::DebugPrint(l.m_matchedTokens.size());
    return res;
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

void Query::SearchAddress()
{
  // Find World.mwm and do special search there.
  MWMVectorT mwmInfo;
  m_pIndex->GetMwmInfo(mwmInfo);

  for (MwmSet::MwmId mwmId = 0; mwmId < mwmInfo.size(); ++mwmId)
  {
    Index::MwmLock mwmLock(*m_pIndex, mwmId);
    MwmValue * pMwm = mwmLock.GetValue();
    if (pMwm &&
        pMwm->m_cont.IsReaderExist(SEARCH_INDEX_FILE_TAG) &&
        pMwm->GetHeader().GetType() == FHeaderT::world)
    {
      impl::Locality city;
      impl::Region region;
      SearchLocality(pMwm, city, region);

      if (city.IsValid())
      {
        LOG(LDEBUG, ("Final city-locality = ", city));

        Params params(*this);
        params.EraseTokens(city.m_matchedTokens);

        if (!params.IsEmpty())
        {
          params.ProcessAddressTokens();

          SetViewportByIndex(mwmInfo, scales::GetRectForLevel(ADDRESS_SCALE, city.m_value.m_pt), ADDRESS_RECT_ID);

          /// @todo Hack - do not search for address in World.mwm; Do it better in future.
          bool const b = m_worldSearch;
          m_worldSearch = false;
          SearchFeatures(params, mwmInfo, ADDRESS_RECT_ID);
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

        Params params(*this);
        params.EraseTokens(region.m_matchedTokens);

        if (!params.IsEmpty())
        {
          for (MwmSet::MwmId id = 0; id < mwmInfo.size(); ++id)
          {
            Index::MwmLock mwmLock(*m_pIndex, id);
            if (m_pInfoGetter->IsBelongToRegion(mwmLock.GetFileName(), region.m_ids))
              SearchInMWM(mwmLock, params);
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

    /// Check for "locality" types.
    class TypeChecker
    {
      vector<uint32_t> m_vec;

    public:
      TypeChecker()
      {
        Classificator const & c = classif();

        char const * arr[][2] = {
          { "place", "country" },
          { "place", "state" },
          { "place", "city" },
          { "place", "town" }
        };

        for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
          m_vec.push_back(c.GetTypeByPath(vector<string>(arr[i], arr[i] + 2)));
      }

      /// @return Feature type and locality type @see Locality::m_index.
      Locality::Type GetLocalityIndex(FeatureType const & f)
      {
        feature::TypesHolder types(f);
        for (size_t i = 0; i < types.Size(); ++i)
        {
          uint32_t t = types[i];
          ftype::TruncValue(t, 2);

          if (t == m_vec[0])
            return Locality::COUNTRY;

          if (t == m_vec[1])
            return Locality::STATE;

          for (int j = 2; j < m_vec.size(); ++j)
            if (t == m_vec[j])
              return Locality::CITY;
        }

        return Locality::NONE;
      }
    };

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
      for (vector<Locality>::const_reverse_iterator i = arr.rbegin(); i != arr.rend(); ++i)
      {
        // no need to check region with empty english name (can't match for polygon)
        if (!i->m_enName.empty() && i->IsSuitable(m_query.m_tokens, m_query.m_prefix))
        {
          vector<size_t> vec;
          m_query.m_pInfoGetter->GetMatchedRegions(i->m_enName, vec);
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
        return m_query.m_pInfoGetter->IsBelongToRegion(loc.m_value.m_pt, r.m_ids);
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
    DoFindLocality(Query & q, MwmValue * pMwm, int8_t lang)
      : m_query(q), m_vector(pMwm->m_cont, pMwm->GetHeader()), m_lang(lang)
    {
      m_arrEn[0] = q.GetLanguage(LANG_EN);
      m_arrEn[1] = q.GetLanguage(LANG_INTERNATIONAL);
      m_arrEn[2] = q.GetLanguage(LANG_DEFAULT);
    }

    void Resize(size_t) {}
    void StartNew(size_t ind) { m_index = ind; }

    void operator() (Query::TrieValueT const & v)
    {
      if (m_query.IsCanceled())
        throw Query::CancelException();

      // find locality in current results
      for (size_t i = 0; i < 3; ++i)
      {
        vector<Locality>::iterator it = find_if(m_localities[i].begin(), m_localities[i].end(),
                                                EqualID(v.m_featureId));
        if (it != m_localities[i].end())
        {
          it->m_matchedTokens.push_back(m_index);
          return;
        }
      }

      // load feature
      FeatureType f;
      m_vector.Get(v.m_featureId, f);

      // check, if feature is locality
      static TypeChecker checker;
      Locality::Type const index = checker.GetLocalityIndex(f);
      if (index != Locality::NONE)
      {
        Locality * loc = PushLocality(Locality(v, index));
        if (loc)
        {
          // m_lang name should exist if we matched feature in search index for this language.
          VERIFY(f.GetName(m_lang, loc->m_name), ());

          loc->m_matchedTokens.push_back(m_index);

          AssignEnglishName(f, *loc);
        }
      }
    }

    void SortLocalities()
    {
      for (int i = Locality::COUNTRY; i <= Locality::CITY; ++i)
        sort(m_localities[i].begin(), m_localities[i].end());
    }

    void GetRegions(vector<Region> & regions) const
    {
      //LOG(LDEBUG, ("Countries before processing = ", m_localities[Locality::COUNTRY]));
      //LOG(LDEBUG, ("States before processing = ", m_localities[Locality::STATE]));

      AddRegions(Locality::STATE, regions);
      AddRegions(Locality::COUNTRY, regions);

      //LOG(LDEBUG, ("Regions after processing = ", regions));
    }

    void GetBestCity(Locality & res, vector<Region> const & regions)
    {
      size_t const regsCount = regions.size();
      vector<Locality> & arr = m_localities[Locality::CITY];

      // Interate in reverse order from better to generic locality.
      for (vector<Locality>::reverse_iterator i = arr.rbegin(); i != arr.rend(); ++i)
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
                                    belongs[j]->m_matchedTokens.begin(), belongs[j]->m_matchedTokens.end());
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

void Query::SearchLocality(MwmValue * pMwm, impl::Locality & res1, impl::Region & res2)
{
  Params params(*this, true);

  serial::CodingParams cp(GetCPForTrie(pMwm->GetHeader().GetDefCodingParams()));

  ModelReaderPtr searchReader = pMwm->m_cont.GetReader(SEARCH_INDEX_FILE_TAG);
  scoped_ptr<TrieIterator> pTrieRoot(::trie::reader::ReadTrie(
                                     SubReaderWrapper<Reader>(searchReader.GetPtr()),
                                     trie::ValueReader(cp),
                                     trie::EdgeValueReader()));

  for (size_t i = 0; i < pTrieRoot->m_edge.size(); ++i)
  {
    TrieIterator::Edge::EdgeStrT const & edge = pTrieRoot->m_edge[i].m_str;

    /// We do search countries, states and cities for one language.
    /// @todo Do combine countries and cities for different languages.
    int8_t const lang = static_cast<int8_t>(edge[0]);
    if (edge[0] < search::CATEGORIES_LANG && params.IsLangExist(lang))
    {
      scoped_ptr<TrieIterator> pLangRoot(pTrieRoot->GoToEdge(i));

      // gel all localities from mwm
      impl::DoFindLocality doFind(*this, pMwm, lang);
      GetFeaturesInTrie(params.m_tokens, params.m_prefixTokens,
                        TrieRootPrefix(*pLangRoot, edge), doFind);

      // sort localities by priority
      doFind.SortLocalities();

      // get Region's from STATE and COUNTRY localities
      vector<impl::Region> regions;
      doFind.GetRegions(regions);

      // get best CITY locality
      impl::Locality loc;
      doFind.GetBestCity(loc, regions);
      if (res1 < loc)
      {
        LOG(LDEBUG, ("Better location ", loc, " for language ", lang));
        res1.Swap(loc);
      }

      // get best region
      if (!regions.empty())
      {
        sort(regions.begin(), regions.end());
        if (res2 < regions.back())
          res2.Swap(regions.back());
      }
    }
  }
}

void Query::SearchFeatures()
{
  MWMVectorT mwmInfo;
  m_pIndex->GetMwmInfo(mwmInfo);

  Params params(*this);

  // do usual search in viewport and near me (without last rect)
  for (size_t i = 0; i < RECTSCOUNT-1; ++i)
  {
    if (m_viewport[i].IsValid())
      SearchFeatures(params, mwmInfo, i);
  }
}

namespace
{
  class FeaturesFilter
  {
    vector<uint32_t> const * m_offsets;

    volatile bool & m_isCancelled;
  public:
    FeaturesFilter(vector<uint32_t> const * offsets, volatile bool & isCancelled)
      : m_offsets(offsets), m_isCancelled(isCancelled)
    {
    }

    bool operator() (uint32_t offset) const
    {
      if (m_isCancelled)
      {
        //LOG(LINFO, ("Throw CancelException"));
        //dbg::ObjectTracker::PrintLeaks();
        throw Query::CancelException();
      }

      return (m_offsets == 0 ||
              binary_search(m_offsets->begin(), m_offsets->end(), offset));
    }
  };

  template <class FilterT> class TrieValuesHolder
  {
    vector<vector<Query::TrieValueT> > m_holder;
    size_t m_ind;
    FilterT const & m_filter;

  public:
    TrieValuesHolder(FilterT const & filter) : m_filter(filter) {}

    void Resize(size_t count) { m_holder.resize(count); }
    void StartNew(size_t ind)
    {
      ASSERT_LESS ( ind, m_holder.size(), () );
      m_ind = ind;
    }
    void operator() (Query::TrieValueT const & v)
    {
      if (m_filter(v.m_featureId))
        m_holder[m_ind].push_back(v);
    }

    template <class ToDo> void GetValues(size_t ind, ToDo & toDo) const
    {
      for (size_t i = 0; i < m_holder[ind].size(); ++i)
        toDo(m_holder[ind][i]);
    }
  };
}

void Query::SearchFeatures(Params const & params, MWMVectorT const & mwmInfo, int ind)
{
  for (MwmSet::MwmId mwmId = 0; mwmId < mwmInfo.size(); ++mwmId)
  {
    // Search only mwms that intersect with viewport (world always does).
    if (m_viewport[ind].IsIntersect(mwmInfo[mwmId].m_limitRect))
    {
      Index::MwmLock mwmLock(*m_pIndex, mwmId);
      SearchInMWM(mwmLock, params, ind);
    }
  }
}

namespace
{

void FillCategories(Query::Params const & params, TrieIterator const * pTrieRoot,
                    TrieValuesHolder<FeaturesFilter> & categoriesHolder)
{
  scoped_ptr<TrieIterator> pCategoriesRoot;
  typedef TrieIterator::Edge::EdgeStrT EdgeT;
  EdgeT categoriesEdge;

  size_t const count = pTrieRoot->m_edge.size();
  for (size_t i = 0; i < count; ++i)
  {
    EdgeT const & edge = pTrieRoot->m_edge[i].m_str;
    ASSERT_GREATER_OR_EQUAL(edge.size(), 1, ());

    if (edge[0] == search::CATEGORIES_LANG)
    {
      categoriesEdge = edge;
      pCategoriesRoot.reset(pTrieRoot->GoToEdge(i));
      break;
    }
  }
  ASSERT_NOT_EQUAL(pCategoriesRoot, 0, ());

  GetFeaturesInTrie(params.m_tokens, params.m_prefixTokens,
                    TrieRootPrefix(*pCategoriesRoot, categoriesEdge),
                    categoriesHolder);
}

}

void Query::SearchInMWM(Index::MwmLock const & mwmLock, Params const & params, int ind/* = -1*/)
{
  if (MwmValue * pMwm = mwmLock.GetValue())
  {
    if (pMwm->m_cont.IsReaderExist(SEARCH_INDEX_FILE_TAG))
    {
      FHeaderT const & header = pMwm->GetHeader();

      /// @todo do not process World.mwm here - do it in SearchLocality
      bool const isWorld = (header.GetType() == FHeaderT::world);
      if (isWorld && !m_worldSearch)
        return;

      serial::CodingParams cp(GetCPForTrie(header.GetDefCodingParams()));

      ModelReaderPtr searchReader = pMwm->m_cont.GetReader(SEARCH_INDEX_FILE_TAG);
      scoped_ptr<TrieIterator> pTrieRoot(::trie::reader::ReadTrie(
                                         SubReaderWrapper<Reader>(searchReader.GetPtr()),
                                         trie::ValueReader(cp),
                                         trie::EdgeValueReader()));

      MwmSet::MwmId const mwmId = mwmLock.GetID();
      FeaturesFilter filter((ind == -1 || isWorld) ? 0 : &m_offsetsInViewport[ind][mwmId], m_cancel);

      // Get categories for each token separately - find needed edge with categories.
      TrieValuesHolder<FeaturesFilter> categoriesHolder(filter);
      FillCategories(params, pTrieRoot.get(), categoriesHolder);

      // Match tokens to feature for each language - iterate through first edges.
      impl::FeatureLoader emitter(*this, mwmId, ind);
      size_t const count = pTrieRoot->m_edge.size();
      for (size_t i = 0; i < count; ++i)
      {
        TrieIterator::Edge::EdgeStrT const & edge = pTrieRoot->m_edge[i].m_str;
        if (edge[0] < search::CATEGORIES_LANG && params.IsLangExist(static_cast<int8_t>(edge[0])))
        {
          scoped_ptr<TrieIterator> pLangRoot(pTrieRoot->GoToEdge(i));

          MatchFeaturesInTrie(params.m_tokens, params.m_prefixTokens,
                              TrieRootPrefix(*pLangRoot, edge),
                              filter, categoriesHolder, emitter);

          LOG(LDEBUG, ("Country", pMwm->GetFileName(),
                       "Lang", StringUtf8Multilang::GetLangByCode(static_cast<int8_t>(edge[0])),
                       "Matched", emitter.GetCount()));

          emitter.Reset();
        }
      }
    }
  }
}

void Query::SuggestStrings(Results & res)
{
  if (m_pStringsToSuggest && !m_prefix.empty())
  {
    switch (m_tokens.size())
    {
    case 0:
      // Match prefix.
      MatchForSuggestions(m_prefix, res);
      break;

    case 1:
      // Match token + prefix.
      strings::UniString tokenAndPrefix = m_tokens[0];
      tokenAndPrefix.push_back(' ');
      tokenAndPrefix.append(m_prefix.begin(), m_prefix.end());

      MatchForSuggestions(tokenAndPrefix, res);
      break;
    }
  }
}

bool Query::MatchForSuggestionsImpl(strings::UniString const & token, int8_t lang, Results & res)
{
  bool ret = false;

  StringsToSuggestVectorT::const_iterator it = m_pStringsToSuggest->begin();
  for (; it != m_pStringsToSuggest->end(); ++it)
  {
    strings::UniString const & s = it->m_name;
    if ((it->m_prefixLength <= token.size()) &&
        (token != s) &&          // do not push suggestion if it already equals to token
        (it->m_lang == lang) &&  // push suggestions only for needed language
        StartsWith(s.begin(), s.end(), token.begin(), token.end()))
    {
      res.AddResult(MakeResult(impl::PreResult2(strings::ToUtf8(s), it->m_prefixLength)));
      ret = true;
    }
  }

  return ret;
}

void Query::MatchForSuggestions(strings::UniString const & token, Results & res)
{
  if (!MatchForSuggestionsImpl(token, GetLanguage(LANG_INPUT), res))
    MatchForSuggestionsImpl(token, GetLanguage(LANG_EN), res);
}

m2::RectD const & Query::GetViewport(int8_t viewportID/* = -1*/) const
{
  if (viewportID == ADDRESS_RECT_ID)
  {
    // special case for search address - return viewport around location
    return m_viewport[viewportID];
  }

  // return first valid actual viewport
  if (m_viewport[0].IsValid())
    return m_viewport[0];
  else
  {
    ASSERT ( m_viewport[1].IsValid(), () );
    return m_viewport[1];
  }
}

m2::PointD Query::GetPosition(int8_t viewportID/* = -1*/) const
{
  if (viewportID == ADDRESS_RECT_ID)
  {
    // special case for search address - return center of location
    return m_viewport[viewportID].Center();
  }
  return m_position;
}

void Query::SearchAllInViewport(m2::RectD const & viewport, Results & res, unsigned int resultsNeeded)
{
  ASSERT ( viewport.IsValid(), () );

  // Get feature's offsets in viewport.
  OffsetsVectorT offsets;
  {
    MWMVectorT mwmInfo;
    m_pIndex->GetMwmInfo(mwmInfo);

    UpdateViewportOffsets(mwmInfo, viewport, offsets);
  }

  vector<shared_ptr<impl::PreResult2> > indV;

  impl::PreResult2Maker maker(*this);

  // load results
  for (size_t i = 0; i < offsets.size(); ++i)
  {
    if (m_cancel) break;

    for (size_t j = 0; j < offsets[i].size(); ++j)
    {
      if (m_cancel) break;

      AddPreResult2(maker(FeatureID(i, offsets[i][j])), indV);
    }
  }

  if (!m_cancel)
  {
    RemoveDuplicatingLinear(indV);

    // sort by distance from m_position
    sort(indV.begin(), indV.end(),
         CompareT<impl::PreResult2, RefPointer>(&impl::PreResult2::LessDistance));

    // emit results
    size_t const count = min(indV.size(), static_cast<size_t>(resultsNeeded));
    for (size_t i = 0; i < count; ++i)
    {
      if (m_cancel) break;

      res.AddResult(MakeResult(*(indV[i])));
    }
  }
}

void Query::SearchAdditional(Results & res, bool nearMe, bool inViewport)
{
  ClearQueues();

  string name[2];

  // search in mwm with position ...
  if (nearMe && m_position.x > empty_pos_value && m_position.y > empty_pos_value)
    name[0] = m_pInfoGetter->GetRegionFile(m_position);

  // ... and in mwm with viewport
  if (inViewport)
    name[1] = m_pInfoGetter->GetRegionFile(GetViewport().Center());

  LOG(LDEBUG, ("Additional MWM search: ", name[0], name[1]));

  if (!(name[0].empty() && name[1].empty()))
  {
    MWMVectorT mwmInfo;
    m_pIndex->GetMwmInfo(mwmInfo);

    Params params(*this);

    for (MwmSet::MwmId mwmId = 0; mwmId < mwmInfo.size(); ++mwmId)
    {
      Index::MwmLock mwmLock(*m_pIndex, mwmId);
      string const s = mwmLock.GetFileName();

      if (s == name[0] || s == name[1])
        SearchInMWM(mwmLock, params);
    }

    FlushResults(res, &Results::AddResultCheckExisting);
  }
}

}  // namespace search
