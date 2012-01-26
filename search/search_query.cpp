#include "search_query.hpp"
#include "category_info.hpp"
#include "feature_offset_match.hpp"
#include "lang_keywords_scorer.hpp"
#include "latlon_match.hpp"
#include "search_common.hpp"

#include "../indexer/feature_covering.hpp"
#include "../indexer/features_vector.hpp"
#include "../indexer/index.hpp"
#include "../indexer/scales.hpp"
#include "../indexer/search_delimiters.hpp"
#include "../indexer/search_string_utils.hpp"

#include "../coding/multilang_utf8_string.hpp"

#include "../base/logging.hpp"
#include "../base/string_utils.hpp"
#include "../base/stl_add.hpp"

#include "../std/algorithm.hpp"
#include "../std/array.hpp"


namespace search
{

Query::Query(Index const * pIndex,
             CategoriesMapT const * pCategories,
             StringsToSuggestVectorT const * pStringsToSuggest,
             storage::CountryInfoGetter const * pInfoGetter)
  : m_pIndex(pIndex),
    m_pCategories(pCategories),
    m_pStringsToSuggest(pStringsToSuggest),
    m_pInfoGetter(pInfoGetter),
    m_preferredLanguage(StringUtf8Multilang::GetLangIndex("en")),
    m_viewport(m2::RectD::GetEmptyRect()), m_viewportExtended(m2::RectD::GetEmptyRect()),
    m_position(-1000000, -1000000),  // initialize as empty point
    m_bOffsetsCacheIsValid(false)
{
}

Query::~Query()
{
}

void Query::SetViewport(m2::RectD const & viewport)
{
  // TODO: Set m_bOffsetsCacheIsValid = false when mwm index is added or removed!

  if (m_viewport != viewport || !m_bOffsetsCacheIsValid)
  {
    m_viewport = viewport;
    m_viewportExtended = m_viewport;
    m_viewportExtended.Scale(3);

    UpdateViewportOffsets();
  }
}

void Query::SetPreferredLanguage(string const & lang)
{
  m_preferredLanguage = StringUtf8Multilang::GetLangIndex(lang);
}

void Query::ClearCache()
{
  m_offsetsInViewport.clear();
  m_bOffsetsCacheIsValid = false;
}

void Query::UpdateViewportOffsets()
{
  m_offsetsInViewport.clear();

  vector<MwmInfo> mwmInfo;
  m_pIndex->GetMwmInfo(mwmInfo);
  m_offsetsInViewport.resize(mwmInfo.size());

  int const viewScale = scales::GetScaleLevel(m_viewport);
  covering::CoveringGetter cov(m_viewport, 0);

  for (MwmSet::MwmId mwmId = 0; mwmId < mwmInfo.size(); ++mwmId)
  {
    // Search only mwms that intersect with viewport (world always does).
    if (m_viewportExtended.IsIntersect(mwmInfo[mwmId].m_limitRect))
    {
      Index::MwmLock mwmLock(*m_pIndex, mwmId);
      if (MwmValue * pMwm = mwmLock.GetValue())
      {
        feature::DataHeader const & header = pMwm->GetHeader();
        if (header.GetType() == feature::DataHeader::country)
        {
          pair<int, int> const scaleR = header.GetScaleRange();
          int const scale = min(max(viewScale + 7, scaleR.first), scaleR.second);

          covering::IntervalsT const & interval = cov.Get(header.GetLastScale());

          ScaleIndex<ModelReaderPtr> index(pMwm->m_cont.GetReader(INDEX_FILE_TAG),
                                           pMwm->m_factory);

          for (size_t i = 0; i < interval.size(); ++i)
          {
            index.ForEachInIntervalAndScale(MakeBackInsertFunctor(m_offsetsInViewport[mwmId]),
                                            interval[i].first, interval[i].second,
                                            scale);
          }

          sort(m_offsetsInViewport[mwmId].begin(), m_offsetsInViewport[mwmId].end());
        }
      }
    }
  }

  m_bOffsetsCacheIsValid = true;

#ifdef DEBUG
  size_t offsetsCached = 0;
  for (MwmSet::MwmId mwmId = 0; mwmId < mwmInfo.size(); ++mwmId)
    offsetsCached += m_offsetsInViewport[mwmId].size();

  LOG(LDEBUG, ("For search in viewport cached ",
              "mwms:", mwmInfo.size(),
              "offsets:", offsetsCached));
#endif
}

namespace
{
  typedef bool (*CompareFunctionT) (impl::IntermediateResult const &,
                                    impl::IntermediateResult const &);
  CompareFunctionT g_arrCompare[] =
  {
    &impl::IntermediateResult::LessRank,
    &impl::IntermediateResult::LessViewportDistance,
    &impl::IntermediateResult::LessDistance
  };
}

void Query::Search(string const & query, Results & res, unsigned int resultsNeeded)
{
  // Initialize.
  {
    m_rawQuery = query;
    m_uniQuery = NormalizeAndSimplifyString(m_rawQuery);
    m_tokens.clear();
    m_prefix.clear();

    search::Delimiters delims;
    SplitUniString(m_uniQuery, MakeBackInsertFunctor(m_tokens), delims);

    if (!m_tokens.empty() && !delims(strings::LastUniChar(m_rawQuery)))
    {
      m_prefix.swap(m_tokens.back());
      m_tokens.pop_back();
    }
    if (m_tokens.size() > 31)
      m_tokens.resize(31);

    vector<vector<int8_t> > langPriorities(3);
    langPriorities[0].push_back(m_preferredLanguage);
    langPriorities[1].push_back(StringUtf8Multilang::GetLangIndex("int_name"));
    langPriorities[1].push_back(StringUtf8Multilang::GetLangIndex("en"));
    langPriorities[2].push_back(StringUtf8Multilang::GetLangIndex("default"));
    m_pKeywordsScorer.reset(new LangKeywordsScorer(langPriorities,
                                                   m_tokens.data(), m_tokens.size(), &m_prefix));

    // Results queue's initialization.
    STATIC_ASSERT ( m_qCount == ARRAY_SIZE(g_arrCompare) );

    for (size_t i = 0; i < m_qCount; ++i)
      m_results[i] = QueueT(resultsNeeded, CompareT(g_arrCompare[i]));
  }

  // Match (lat, lon).
  {
    double lat, lon, latPrec, lonPrec;
    if (search::MatchLatLon(m_rawQuery, lat, lon, latPrec, lonPrec))
    {
      double const precision = 5.0 * max(0.0001, min(latPrec, lonPrec));  // Min 55 meters
      AddResult(ValueT(new impl::IntermediateResult(m_viewport, m_position, lat, lon, precision)));
    }
  }

  SuggestStrings();

  SearchFeatures();

  FlushResults(res);
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
}

void Query::AddResult(ValueT const & result)
{
  for (size_t i = 0; i < m_qCount; ++i)
  {
    // don't add equal features
    if (m_results[i].end() == find_if(m_results[i].begin(), m_results[i].end(),
                                      ProxyFunctor1<ResultT::StrictEqualF>(result)))
    {
      m_results[i].push(result);
    }
    //else
    //{
    //  LOG(LINFO, ("Skipped feature:", result));
    //}
  }
}

namespace
{
  class IndexedValue
  {
    typedef impl::IntermediateResult ValueT;
    typedef shared_ptr<ValueT> PointerT;

    array<size_t, Query::m_qCount> m_ind;
    PointerT m_val;

  public:
    IndexedValue(PointerT const & v) : m_val(v)
    {
      for (size_t i = 0; i < m_ind.size(); ++i)
        m_ind[i] = numeric_limits<size_t>::max();
    }

    PointerT get() const { return m_val; }
    ValueT const & operator*() const { return *m_val; }

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

      return boost::DebugPrint(m_val) + "; Index:" + index;
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

  struct LessPointer
  {
    bool operator() (IndexedValue const & r1, IndexedValue const & r2) const
    {
      return (r1.get() < r2.get());
    }
  };
  struct EqualPointer
  {
    bool operator() (IndexedValue const & r1, IndexedValue const & r2) const
    {
      return (r1.get() == r2.get());
    }
  };

  inline string DebugPrint(IndexedValue const & v)
  {
    return v.DebugPrint();
  }
}

void Query::FlushResults(Results & res)
{
  vector<IndexedValue> indV;

  // fill vector with objects in queues
  for (size_t i = 0; i < m_qCount; ++i)
  {
    indV.insert(indV.end(), m_results[i].begin(), m_results[i].end());
    m_results[i].clear();
  }

  // remove equal objects
  sort(indV.begin(), indV.end(), LessPointer());
  indV.erase(unique(indV.begin(), indV.end(), EqualPointer()), indV.end());

  // remove duplicating linear objects
  sort(indV.begin(), indV.end(), ProxyFunctor2<ResultT::LessLinearTypesF>());
  indV.erase(unique(indV.begin(), indV.end(), ProxyFunctor2<ResultT::EqualLinearTypesF>()),
             indV.end());

  for (size_t i = 0; i < m_qCount; ++i)
  {
    CompareT comp(g_arrCompare[i]);

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

  // emit results
  for (size_t i = 0; i < indV.size(); ++i)
  {
    LOG(LDEBUG, (indV[i]));
    res.AddResult(indV[i].get()->GenerateFinalResult(m_pInfoGetter));
  }
}

void Query::AddFeatureResult(FeatureType const & f, string const & fName)
{
  uint32_t penalty;
  string name;
  GetBestMatchName(f, penalty, name);

  AddResult(ValueT(new impl::IntermediateResult(m_viewport, m_position, f, name, fName)));
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
  f.ForEachNameRef(bestNameFinder);
}

namespace impl
{

class FeatureLoader
{
  size_t m_count;
  FeaturesVector & m_featuresVector;
  Query & m_query;
  string m_fName;

  /*
  class DoFindByName
  {
    string m_name;

  public:
    DoFindByName(char const * s) : m_name(s) {}

    bool operator() (uint8_t, string const & s)
    {
      if (m_name.find_first_of(s) != string::npos)
        LOG(LINFO, ("Found name: ", s));
      return true;
    }
  };
  */

public:
  FeatureLoader(FeaturesVector & featuresVector, Query & query, string const & fName)
    : m_count(0), m_featuresVector(featuresVector), m_query(query), m_fName(fName)
  {
  }

  void operator() (uint32_t offset)
  {
    ++m_count;
    FeatureType feature;
    m_featuresVector.Get(offset, feature);

//#ifdef DEBUG
//    DoFindByName doFind("ул. Карбышева");
//    feature.ForEachNameRef(doFind);
//#endif

    m_query.AddFeatureResult(feature, m_fName);
  }

  size_t GetCount() const { return m_count; }
  void Reset() { m_count = 0; }
};

}  // namespace search::impl

void Query::SearchFeatures()
{
  if (!m_pIndex)
    return;

  vector<vector<strings::UniString> > tokens(m_tokens.size());

  // Add normal tokens.
  for (size_t i = 0; i < m_tokens.size(); ++i)
    tokens[i].push_back(m_tokens[i]);

  // Add names of categories.
  if (m_pCategories)
  {
    for (size_t i = 0; i < m_tokens.size(); ++i)
    {
      typedef CategoriesMapT::const_iterator IterT;

      pair<IterT, IterT> const range = m_pCategories->equal_range(m_tokens[i]);
      for (IterT it = range.first; it != range.second; ++it)
        tokens[i].push_back(FeatureTypeToString(it->second));
    }
  }

  vector<MwmInfo> mwmInfo;
  m_pIndex->GetMwmInfo(mwmInfo);

  unordered_set<int8_t> langs;
  langs.insert(m_preferredLanguage);
  langs.insert(StringUtf8Multilang::GetLangIndex("int_name"));
  langs.insert(StringUtf8Multilang::GetLangIndex("en"));
  langs.insert(StringUtf8Multilang::GetLangIndex("default"));
  SearchFeatures(tokens, mwmInfo, langs, true);
}

namespace
{
  class FeaturesFilter
  {
    vector<uint32_t> const & m_offsets;
    bool m_alwaysTrue;
  public:
    FeaturesFilter(vector<uint32_t> const & offsets, bool alwaysTrue)
      : m_offsets(offsets), m_alwaysTrue(alwaysTrue)
    {
    }

    bool operator() (uint32_t offset) const
    {
      return (m_alwaysTrue || binary_search(m_offsets.begin(), m_offsets.end(), offset));
    }
  };
}

void Query::SearchFeatures(vector<vector<strings::UniString> > const & tokens,
                           vector<MwmInfo> const & mwmInfo,
                           unordered_set<int8_t> const & langs,
                           bool onlyInViewport)
{
  for (MwmSet::MwmId mwmId = 0; mwmId < mwmInfo.size(); ++mwmId)
  {
    // Search only mwms that intersect with viewport (world always does).
    if (!onlyInViewport ||
        m_viewportExtended.IsIntersect(mwmInfo[mwmId].m_limitRect))
    {
      Index::MwmLock mwmLock(*m_pIndex, mwmId);
      if (MwmValue * pMwm = mwmLock.GetValue())
      {
        if (pMwm->m_cont.IsReaderExist(SEARCH_INDEX_FILE_TAG))
        {
          scoped_ptr<TrieIterator> pTrieRoot(::trie::reader::ReadTrie(
                                               pMwm->m_cont.GetReader(SEARCH_INDEX_FILE_TAG),
                                               ::search::trie::ValueReader(),
                                               ::search::trie::EdgeValueReader()));
          if (pTrieRoot)
          {
            feature::DataHeader const & header = pMwm->GetHeader();
            bool const isWorld = (header.GetType() == feature::DataHeader::world);

            FeaturesVector featuresVector(pMwm->m_cont, header);
            impl::FeatureLoader emitter(featuresVector, *this, isWorld ? "" : mwmLock.GetCountryName());

            for (size_t i = 0; i < pTrieRoot->m_edge.size(); ++i)
            {
              TrieIterator::Edge::EdgeStrT const & edge = pTrieRoot->m_edge[i].m_str;
              ASSERT_GREATER_OR_EQUAL(edge.size(), 1, ());

              if (edge.size() >= 1 && edge[0] < 128 && langs.count(static_cast<int8_t>(edge[0])))
              {
                scoped_ptr<TrieIterator> pLangRoot(pTrieRoot->GoToEdge(i));

                MatchFeaturesInTrie(tokens, m_prefix, *pLangRoot,
                                    edge.size() == 1 ? NULL : &edge[1], edge.size() - 1,
                                    FeaturesFilter(m_offsetsInViewport[mwmId], isWorld), emitter);

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
    }
  }
}

void Query::SuggestStrings()
{
  if (m_pStringsToSuggest)
  {
    if (m_tokens.size() == 0 && !m_prefix.empty())
    {
      // Match prefix.
      MatchForSuggestions(m_prefix);
    }
    else if (m_tokens.size() == 1)
    {
      // Match token + prefix.
      strings::UniString tokenAndPrefix = m_tokens[0];
      if (!m_prefix.empty())
      {
        tokenAndPrefix.push_back(' ');
        tokenAndPrefix.append(m_prefix.begin(), m_prefix.end());
      }

      MatchForSuggestions(tokenAndPrefix);
    }
  }
}

void Query::MatchForSuggestions(strings::UniString const & token)
{
  StringsToSuggestVectorT::const_iterator it = m_pStringsToSuggest->begin();
  for (; it != m_pStringsToSuggest->end(); ++it)
  {
    strings::UniString const & s = it->first;
    if (it->second <= token.size() && StartsWith(s.begin(), s.end(), token.begin(), token.end()))
      AddResult(ValueT(new impl::IntermediateResult(strings::ToUtf8(s), it->second)));
  }
}

}  // namespace search
