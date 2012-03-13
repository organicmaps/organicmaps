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

  SetPreferredLanguage("en");

  // Results queue's initialization.
  STATIC_ASSERT ( m_qCount == ARRAY_SIZE(g_arrCompare1) );
  STATIC_ASSERT ( m_qCount == ARRAY_SIZE(g_arrCompare2) );

  for (size_t i = 0; i < m_qCount; ++i)
  {
    m_results[i] = QueueT(2 * resultsNeeded, QueueCompareT(g_arrCompare1[i]));
    m_results[i].reserve(2 * resultsNeeded);
  }
}

Query::~Query()
{
}

namespace
{
  inline bool IsEqualMercator(m2::RectD const & r1, m2::RectD const & r2, double epsMeters)
  {
    double const eps = epsMeters * MercatorBounds::degreeInMetres;

    m2::RectD r = r1;
    r.Inflate(eps, eps);
    if (!r.IsRectInside(r2)) return false;

    r = r2;
    r.Inflate(eps, eps);
    if (!r.IsRectInside(r1)) return false;

    return true;
  }
}

void Query::SetViewport(m2::RectD viewport[], size_t count)
{
  ASSERT_LESS_OR_EQUAL(count, static_cast<size_t>(RECTSCOUNT), ());

  MWMVectorT mwmInfo;
  m_pIndex->GetMwmInfo(mwmInfo);

  for (size_t i = 0; i < count; ++i)
  {
    if (viewport[i].IsValid())
    {
      // Check if viewports are equal (10 meters).
      if (!m_viewport[i].IsValid() || !IsEqualMercator(m_viewport[i], viewport[i], 10.0))
      {
        m_viewport[i] = viewport[i];
        UpdateViewportOffsets(mwmInfo, viewport[i], m_offsetsInViewport[i]);
      }
    }
    else
      ClearCache(i);
  }
}

void Query::SetPreferredLanguage(string const & lang)
{
  m_currentLang = StringUtf8Multilang::GetLangIndex(lang);

  // Default initialization.
  // If you want to reset input language, call SetInputLanguage before search.
  m_inputLang = m_currentLang;
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
          int const scale = min(max(viewScale + m_scaleDepthSearch, scaleR.first), scaleR.second);

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
  m_rawQuery = query;
  m_uniQuery = NormalizeAndSimplifyString(m_rawQuery);
  m_tokens.clear();
  m_prefix.clear();
}

void Query::InitKeywordsScorer()
{
  vector<vector<int8_t> > langPriorities(4);
  langPriorities[0].push_back(m_currentLang);
  langPriorities[1].push_back(m_inputLang);
  langPriorities[2].push_back(StringUtf8Multilang::GetLangIndex("int_name"));
  langPriorities[2].push_back(StringUtf8Multilang::GetLangIndex("en"));
  langPriorities[3].push_back(StringUtf8Multilang::GetLangIndex("default"));
  m_pKeywordsScorer.reset(new LangKeywordsScorer(langPriorities,
                                                 m_tokens.data(), m_tokens.size(), &m_prefix));
}

void Query::ClearQueues()
{
  for (size_t i = 0; i < m_qCount; ++i)
    m_results[i].clear();
}

void Query::Search(string const & query, Results & res, unsigned int resultsNeeded)
{
  // Initialize.

  InitSearch(query);

  search::Delimiters delims;
  SplitUniString(m_uniQuery, MakeBackInsertFunctor(m_tokens), delims);

  if (!m_tokens.empty() && !delims(strings::LastUniChar(m_rawQuery)))
  {
    m_prefix.swap(m_tokens.back());
    m_tokens.pop_back();
  }
  if (m_tokens.size() > 31)
    m_tokens.resize(31);

  InitKeywordsScorer();

  ClearQueues();

  // Match (lat, lon).
  {
    double lat, lon, latPrec, lonPrec;
    if (search::MatchLatLon(m_rawQuery, lat, lon, latPrec, lonPrec))
    {
      //double const precision = 5.0 * max(0.0001, min(latPrec, lonPrec));  // Min 55 meters
      res.AddResult(MakeResult(impl::PreResult2(GetViewport(), m_position, lat, lon)));
    }
  }

  if (m_cancel) return;
  SuggestStrings(res);

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
    IndexedValue(value_type * v) : m_val(v)
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
        indV.push_back(p);
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

    // For the best performance, impl::PreResult1 should be sorted by impl::PreResult1::GetID().
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

void Query::AddResultFromTrie(TrieValueT const & val, size_t mwmID)
{
  impl::PreResult1 res(val.m_featureId, val.m_rank, val.m_pt, mwmID, m_position, GetViewport());

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
  LangKeywordsScorer & m_keywordsScorer;
public:
  BestNameFinder(uint32_t & penalty, string & name, LangKeywordsScorer & keywordsScorer)
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

void Query::GetBestMatchName(FeatureType const & f, uint32_t & penalty, string & name)
{
  impl::BestNameFinder bestNameFinder(penalty, name, *m_pKeywordsScorer);
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

namespace impl
{

class FeatureLoader
{
  Query & m_query;
  size_t m_mwmID;
  size_t m_count;

public:
  FeatureLoader(Query & query, size_t mwmID)
    : m_query(query), m_mwmID(mwmID), m_count(0)
  {
  }

  void operator() (Query::TrieValueT const & value)
  {
    ++m_count;
    m_query.AddResultFromTrie(value, m_mwmID);
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

Query::Params::Params(Query & q)
{
  if (!q.m_prefix.empty())
    m_prefixTokens.push_back(q.m_prefix);

  size_t const tokensCount = q.m_tokens.size();
  m_tokens.resize(tokensCount);

  // Add normal tokens.
  for (size_t i = 0; i < tokensCount; ++i)
    m_tokens[i].push_back(q.m_tokens[i]);

  // Add names of categories.
  if (q.m_pCategories)
  {
    for (size_t i = 0; i < tokensCount; ++i)
      q.m_pCategories->ForEachTypeByName(q.m_tokens[i], DoInsertTypeNames(m_tokens[i]));

    if (!q.m_prefix.empty())
      q.m_pCategories->ForEachTypeByName(q.m_prefix, DoInsertTypeNames(m_prefixTokens));
  }

  m_langs.insert(q.m_currentLang);
  m_langs.insert(q.m_inputLang);
  m_langs.insert(StringUtf8Multilang::GetLangIndex("int_name"));
  m_langs.insert(StringUtf8Multilang::GetLangIndex("en"));
  m_langs.insert(StringUtf8Multilang::GetLangIndex("default"));
}

void Query::SearchFeatures()
{
  MWMVectorT mwmInfo;
  m_pIndex->GetMwmInfo(mwmInfo);

  Params params(*this);
  for (size_t i = 0; i < RECTSCOUNT; ++i)
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
        throw Query::CancelException();

      return (m_offsets == 0 ||
              binary_search(m_offsets->begin(), m_offsets->end(), offset));
    }
  };
}

void Query::SearchFeatures(Params const & params,
                           MWMVectorT const & mwmInfo,
                           size_t ind)
{
  for (MwmSet::MwmId mwmId = 0; mwmId < mwmInfo.size(); ++mwmId)
  {
    // Search only mwms that intersect with viewport (world always does).
    if (m_viewport[ind].IsIntersect(mwmInfo[mwmId].m_limitRect))
    {
      Index::MwmLock mwmLock(*m_pIndex, mwmId);
      SearchInMWM(mwmLock, params, &m_offsetsInViewport[ind]);
    }
  }
}

void Query::SearchInMWM(Index::MwmLock const & mwmLock, Params const & params,
                        OffsetsVectorT const * offsets)
{
  if (MwmValue * pMwm = mwmLock.GetValue())
  {
    if (pMwm->m_cont.IsReaderExist(SEARCH_INDEX_FILE_TAG))
    {
      feature::DataHeader const & header = pMwm->GetHeader();
      bool const isWorld = (header.GetType() == feature::DataHeader::world);

      if (isWorld && !m_worldSearch)
        return;

      serial::CodingParams cp(GetCPForTrie(header.GetDefCodingParams()));

      ModelReaderPtr searchReader = pMwm->m_cont.GetReader(SEARCH_INDEX_FILE_TAG);
      scoped_ptr<TrieIterator> pTrieRoot(::trie::reader::ReadTrie(
                                         SubReaderWrapper<Reader>(searchReader.GetPtr()),
                                         trie::ValueReader(cp),
                                         trie::EdgeValueReader()));

      // Get categories edge root.
      scoped_ptr<TrieIterator> pCategoriesRoot;
      TrieIterator::Edge::EdgeStrT categoriesEdge;

      size_t const count = pTrieRoot->m_edge.size();
      for (size_t i = 0; i < count; ++i)
      {
        TrieIterator::Edge::EdgeStrT const & edge = pTrieRoot->m_edge[i].m_str;
        ASSERT_GREATER_OR_EQUAL(edge.size(), 1, ());

        if (edge[0] == search::CATEGORIES_LANG)
        {
          categoriesEdge = edge;
          pCategoriesRoot.reset(pTrieRoot->GoToEdge(i));
          break;
        }
      }
      ASSERT_NOT_EQUAL(pCategoriesRoot, 0, ());

      MwmSet::MwmId const mwmId = mwmLock.GetID();
      impl::FeatureLoader emitter(*this, mwmId);

      // Iterate through first language edges.
      for (size_t i = 0; i < count; ++i)
      {
        TrieIterator::Edge::EdgeStrT const & edge = pTrieRoot->m_edge[i].m_str;
        if (edge[0] < search::CATEGORIES_LANG && params.m_langs.count(static_cast<int8_t>(edge[0])))
        {
          scoped_ptr<TrieIterator> pLangRoot(pTrieRoot->GoToEdge(i));

          MatchFeaturesInTrie(params.m_tokens, params.m_prefixTokens,
                              TrieRootPrefix(*pLangRoot, edge),
                              TrieRootPrefix(*pCategoriesRoot, categoriesEdge),
                              FeaturesFilter((isWorld || offsets == 0) ? 0 : &((*offsets)[mwmId]),
                                             m_cancel),
                              emitter);

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
  if (!MatchForSuggestionsImpl(token, m_inputLang, res))
    MatchForSuggestionsImpl(token, StringUtf8Multilang::GetLangIndex("en"), res);
}

m2::RectD const & Query::GetViewport() const
{
  if (m_viewport[0].IsValid())
    return m_viewport[0];
  else
  {
    ASSERT ( m_viewport[1].IsValid(), () );
    return m_viewport[1];
  }
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

  // Init search.
  InitSearch(string());
  InitKeywordsScorer();

  vector<impl::PreResult2*> indV;

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

  for_each(indV.begin(), indV.end(), DeleteFunctor());
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
        SearchInMWM(mwmLock, params, 0);
      }
    }

    FlushResults(res, &Results::AddResultCheckExisting);
  }
}

}  // namespace search
