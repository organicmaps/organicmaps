#include "search_query.hpp"
#include "feature_offset_match.hpp"
#include "lang_keywords_scorer.hpp"
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

void Query::ClearCache()
{
  for (size_t i = 0; i < RECTSCOUNT; ++i)
    ClearCache(i);
}

void Query::ClearCache(size_t ind)
{
  m_offsetsInViewport[ind].clear();
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
        feature::DataHeader const & header = pMwm->GetHeader();
        if (header.GetType() == feature::DataHeader::country)
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

void Query::InitSearch(string const & query)
{
  m_cancel = false;
  m_tokens.clear();
  m_prefix.clear();
}

void Query::ClearQueues()
{
  for (size_t i = 0; i < m_qCount; ++i)
    m_results[i].clear();
}

void Query::Search(string const & query, Results & res)
{
  InitSearch(query);

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
  m_keywordsScorer.SetKeywords(m_tokens.data(), m_tokens.size(), &m_prefix);

  ClearQueues();

  // Match (lat, lon).
  {
    double lat, lon, latPrec, lonPrec;
    if (search::MatchLatLon(query, lat, lon, latPrec, lonPrec))
    {
      //double const precision = 5.0 * max(0.0001, min(latPrec, lonPrec));  // Min 55 meters
      res.AddResult(MakeResult(impl::PreResult2(GetViewport(), m_position, lat, lon)));
    }
  }

  if (m_cancel) return;
  SuggestStrings(res);

  if (m_cancel) return;
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
     typedef impl::PreResult2 value_type;

  private:
    array<size_t, Query::m_qCount> m_ind;

    /// @todo Do not use shared_ptr for optimization issues.
    /// Need to rewrite std::unique algorithm.
    shared_ptr<value_type> m_val;

  public:
    explicit IndexedValue(value_type * v) : m_val(v)
    {
      for (size_t i = 0; i < m_ind.size(); ++i)
        m_ind[i] = numeric_limits<size_t>::max();
    }

    value_type const & operator*() const { return *m_val; }

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
    struct LockedFeaturesVector
    {
      Index::MwmLock m_lock;
      FeaturesVector m_vector;

      // Assume that we didn't remove maps during search, so m_lock.GetValue() != 0.
      LockedFeaturesVector(Index const & index, MwmSet::MwmId const & id)
        : m_lock(index, id), m_vector(m_lock.GetContainer(), m_lock.GetHeader())
      {
      }

      string GetCountry() const
      {
        if (m_lock.GetHeader().GetType() == feature::DataHeader::world)
          return string();
        return m_lock.GetCountryName();
      }

      MwmSet::MwmId GetID() const { return m_lock.GetID(); }
    };

    Query & m_query;

    scoped_ptr<LockedFeaturesVector> m_pFV;

    // For the best performance, incoming id's should be sorted by id.first (mwm file id).
    void LoadFeature(pair<size_t, uint32_t> const & id,
                     FeatureType & f, string & name, string & country)
    {
      if (m_pFV.get() == 0 || m_pFV->GetID() != id.first)
        m_pFV.reset(new LockedFeaturesVector(*m_query.m_pIndex, id.first));

      m_pFV->m_vector.Get(id.second, f);

      uint32_t penalty;
      m_query.GetBestMatchName(f, penalty, name);

      country = m_pFV->GetCountry();
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

      return new impl::PreResult2(feature, res, name, country);
    }

    impl::PreResult2 * operator() (pair<size_t, uint32_t> const & id)
    {
      FeatureType feature;
      string name, country;
      LoadFeature(id, feature, name, country);

      if (!name.empty() && !country.empty())
      {
        // this results will be with equal rank
        impl::PreResult1 res(0, 0, feature.GetLimitRect(FeatureType::WORST_GEOMETRY).Center(),
                             0, m_query.m_position, m_query.GetViewport());
        return new impl::PreResult2(feature, res, name, country);
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

void Query::AddResultFromTrie(TrieValueT const & val, size_t mwmID, int viewportID)
{
  impl::PreResult1 res(val.m_featureId, val.m_rank, val.m_pt, mwmID,
                       GetPosition(viewportID), GetViewport(viewportID));

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
  uint32_t & m_penalty;
  string & m_name;
  LangKeywordsScorer const & m_keywordsScorer;
public:
  BestNameFinder(uint32_t & penalty, string & name, LangKeywordsScorer const & keywordsScorer)
    : m_penalty(penalty), m_name(name), m_keywordsScorer(keywordsScorer)
  {
    m_penalty = uint32_t(-1);
  }

  bool operator()(signed char lang, string const & name) const
  {
    uint32_t penalty = m_keywordsScorer.Score(lang, name);
    if (penalty < m_penalty)
    {
      m_penalty = penalty;
      m_name = name;
    }
    return true;
  }
};

}  // namespace search::impl

void Query::GetBestMatchName(FeatureType const & f, uint32_t & penalty, string & name) const
{
  impl::BestNameFinder bestNameFinder(penalty, name, m_keywordsScorer);
  (void)f.ForEachNameRef(bestNameFinder);

  /*
  if (!f.ForEachNameRef(bestNameFinder))
  {
    feature::TypesHolder types(f);
    LOG(LDEBUG, (types));
    LOG(LDEBUG, (f.GetLimitRect(FeatureType::BEST_GEOMETRY)));
  }
  */
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
  int m_viewportID;

public:
  FeatureLoader(Query & query, size_t mwmID, int viewportID)
    : m_query(query), m_mwmID(mwmID), m_count(0), m_viewportID(viewportID)
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
  TokensVectorT & m_tokens;

public:
  DoInsertTypeNames(TokensVectorT & tokens) : m_tokens(tokens) {}
  void operator() (uint32_t t)
  {
    m_tokens.push_back(FeatureTypeToString(t));
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
      q.m_pCategories->ForEachTypeByName(q.m_tokens[i], DoInsertTypeNames(m_tokens[i]));

    if (!q.m_prefix.empty())
      q.m_pCategories->ForEachTypeByName(q.m_prefix, DoInsertTypeNames(m_prefixTokens));
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

namespace
{
  bool IsNumber(strings::UniString const & s)
  {
    for (size_t i = 0; i < s.size(); ++i)
      if (!isdigit(s[i]))
        return false;
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
  // Assume that USA street name numbers endswith "st, nd, rd, th" suffixes.

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
    int m_index;                    ///< locality index (0 - USA state, 1 - country, 2 - city)

    Locality() : m_index(-1) {}
    Locality(Query::TrieValueT const & val, int index) : m_value(val), m_index(index) {}

    void Swap(Locality & rhs)
    {
      m_name.swap(rhs.m_name);
      m_enName.swap(rhs.m_enName);
      swap(m_value, rhs.m_value);
      m_matchedTokens.swap(rhs.m_matchedTokens);
      swap(m_index, rhs.m_index);
    }

    bool operator< (Locality const & rhs) const
    {
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
      switch (m_index)
      {
      case 0:   // USA state
        // USA states has 2-symbol synonyms
        return (isMatched || GetSynonymTokenLength(tokens, prefix) == 2);

      case 1:   // country
        // USA has synonyms: "US" or "USA"
        return (isMatched || (m_enName == "usa" && GetSynonymTokenLength(tokens, prefix) <= 3));

      case 2:   // city
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
  };

  string DebugPrint(Region const & r)
  {
    return string("Name: " + r.m_enName);
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
        pMwm->GetHeader().GetType() == feature::DataHeader::world)
    {
      impl::Locality loc;
      if (SearchLocality(pMwm, loc))
      {
        LOG(LDEBUG, ("Final locality = ", loc));

        Params params(*this);
        params.EraseTokens(loc.m_matchedTokens);

        if (!params.IsEmpty())
        {
          params.ProcessAddressTokens();

          SetViewportByIndex(mwmInfo, scales::GetRectForLevel(ADDRESS_SCALE, loc.m_value.m_pt, 1.0), ADDRESS_RECT_ID);

          /// @todo Hack - do not search for address in World.mwm; Do it better in future.
          bool const b = m_worldSearch;
          m_worldSearch = false;
          SearchFeatures(params, mwmInfo, ADDRESS_RECT_ID);
          m_worldSearch = b;
        }
        else
        {
          // Add found locality as a result if nothing left to match.
          AddResultFromTrie(loc.m_value, mwmId);
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

    /// @see Locality::m_index
    vector<Locality> m_localities[3];

    FeaturesVector m_vector;
    size_t m_index;         ///< index of processing token

    class TypeChecker
    {
      vector<uint32_t> m_vec;

    public:
      TypeChecker()
      {
        char const * arr[][2] = {
          { "place", "country" },
          { "place", "city" },
          { "place", "town" }
        };

        Classificator const & c = classif();

        // process only USA states
        char const * arrStateUSA[] = { "place", "state", "USA" };
        m_vec.push_back(c.GetTypeByPath(vector<string>(arrStateUSA, arrStateUSA + 3)));

        for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
          m_vec.push_back(c.GetTypeByPath(vector<string>(arr[i], arr[i] + 2)));
      }

      /// @return Feature type and locality type @see Locality::m_index.
      int GetLocalityIndex(FeatureType const & f)
      {
        feature::TypesHolder types(f);
        for (size_t i = 0; i < types.Size(); ++i)
        {
          uint32_t t = types[i];

          ftype::TruncValue(t, 3);
          if (t == m_vec[0])
            return 0;

          ftype::TruncValue(t, 2);
          for (int j = 1; j < m_vec.size(); ++j)
            if (t == m_vec[j])
              return (j == 1 ? 1 : 2);
        }

        return -1;
      }
    };

    Locality * PushLocality(Locality const & l)
    {
      ASSERT_LESS ( l.m_index, ARRAY_SIZE(m_localities), () );
      m_localities[l.m_index].push_back(l);
      return &(m_localities[l.m_index].back());
    }

    int8_t m_lang;
    int8_t m_arrEn[3];

    void AssignEnglishName(FeatureType const & f, Locality & l)
    {
      // search for name in next order: "en", "int_name", "default"
      for (int i = 0; i < 3; ++i)
        if (f.GetName(m_arrEn[i], l.m_enName))
        {
          // make name lower-case
          strings::AsciiToLower(l.m_enName);

          // do special correction for USA
          // (needed for CountryInfoGetter::GetMatchedRegions matching)
          char const * arrUSA[] = { "united", "states", "america" };
          for (size_t i = 0; i < ARRAY_SIZE(arrUSA); ++i)
            if (l.m_enName.find(arrUSA[i]) == string::npos)
              return;

          l.m_enName = "usa";
          return;
        }
    }

    void AddRegions(int index, vector<Region> & regions)
    {
      // fill regions vector in priority order
      vector<Locality> & arr = m_localities[index];
      sort(arr.begin(), arr.end());

      for (vector<Locality>::reverse_iterator i = arr.rbegin(); i != arr.rend(); ++i)
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
            r.m_matchedTokens.swap(i->m_matchedTokens);
            r.m_enName.swap(i->m_enName);
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
      int const index = checker.GetLocalityIndex(f);
      if (index >= 0)
      {
        Locality * loc = PushLocality(Locality(v, index));
        if (loc)
        {
          f.GetName(m_lang, loc->m_name);

          loc->m_matchedTokens.push_back(m_index);

          AssignEnglishName(f, *loc);
        }
      }
    }

    bool GetBestLocality(Locality & res)
    {
      LOG(LDEBUG, ("Countries before processing = ", m_localities[1]));
      LOG(LDEBUG, ("States before processing = ", m_localities[0]));

      // First, prepare countries info for checking locality.
      vector<Region> regions;
      AddRegions(1, regions);
      AddRegions(0, regions);

      LOG(LDEBUG, ("Regions after processing = ", regions));

      // Get suitable locality.
      vector<Locality> & arr = m_localities[2];
      sort(arr.begin(), arr.end());

      Locality * p1st = 0;
      for (vector<Locality>::reverse_iterator i = arr.rbegin(); i != arr.rend(); ++i)
      {
        if (!i->IsSuitable(m_query.m_tokens, m_query.m_prefix))
          continue;

        if (!regions.empty())
        {
          // additional check for locality belongs to region
          Region const * p = 0;
          for (size_t j = 0; j < regions.size(); ++j)
          {
            if (IsBelong(*i, regions[j]))
            {
              p = &regions[j];
              break;
            }
          }

          if (p == 0)
          {
            // remember first suitable locality for the last chance
            if (p1st == 0)
              p1st = &(*i);

            // locality doesn't belong to region - goto next
            continue;
          }
          else
          {
            // splice locality info with region info
            i->m_matchedTokens.insert(i->m_matchedTokens.end(),
                                      p->m_matchedTokens.begin(), p->m_matchedTokens.end());
            i->m_enName = i->m_enName + ", " + p->m_enName;
          }
        }

        // get best locality and exit
        i->Swap(res);
        return true;
      }

      if (p1st)
      {
        p1st->Swap(res);
        return true;
      }
      else
        return false;
    }
  };
}

bool Query::SearchLocality(MwmValue * pMwm, impl::Locality & res)
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
    int8_t const lang = static_cast<int8_t>(edge[0]);
    if (edge[0] < search::CATEGORIES_LANG && params.IsLangExist(lang))
    {
      scoped_ptr<TrieIterator> pLangRoot(pTrieRoot->GoToEdge(i));

      impl::DoFindLocality doFind(*this, pMwm, lang);
      GetFeaturesInTrie(params.m_tokens, params.m_prefixTokens,
                        TrieRootPrefix(*pLangRoot, edge), doFind);

      // save better locality, if any
      impl::Locality loc;
      if (doFind.GetBestLocality(loc) && (res < loc))
      {
        LOG(LDEBUG, ("Better location ", loc, " for language ", lang));
        res = loc;
      }
    }
  }

  return (res.m_index >= 0);
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
      feature::DataHeader const & header = pMwm->GetHeader();

      /// @todo do not process World.mwm here - do it in SearchLocality
      bool const isWorld = (header.GetType() == feature::DataHeader::world);
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

          LOG(LDEBUG, ("Lang:",
                       StringUtf8Multilang::GetLangByCode(static_cast<int8_t>(edge[0])),
                       "Matched: ",
                       emitter.GetCount()));

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

m2::RectD const & Query::GetViewport(int viewportID/* = -1*/) const
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

m2::PointD Query::GetPosition(int viewportID/* = -1*/) const
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

  InitSearch(string());

  vector<shared_ptr<impl::PreResult2> > indV;

  impl::PreResult2Maker maker(*this);

  // load results
  for (size_t i = 0; i < offsets.size(); ++i)
  {
    if (m_cancel) break;

    for (size_t j = 0; j < offsets[i].size(); ++j)
    {
      if (m_cancel) break;

      AddPreResult2(maker(make_pair(i, offsets[i][j])), indV);
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

void Query::SearchAdditional(Results & res)
{
  string name[2];

  // search in mwm with position ...
  if (m_position.x > empty_pos_value && m_position.y > empty_pos_value)
    name[0] = m_pInfoGetter->GetRegionFile(m_position);

  // ... and in mwm with viewport
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
      string const s = mwmLock.GetCountryName();

      if (s == name[0] || s == name[1])
      {
        ClearQueues();
        SearchInMWM(mwmLock, params);
      }
    }

    FlushResults(res, &Results::AddResultCheckExisting);
  }
}

}  // namespace search
