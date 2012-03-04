#pragma once
#include "intermediate_result.hpp"

#include "../indexer/search_trie.hpp"
#include "../indexer/index.hpp"   // for Index::MwmLock

#include "../geometry/rect2d.hpp"

#include "../base/buffer_vector.hpp"
#include "../base/limited_priority_queue.hpp"
#include "../base/string_utils.hpp"

#include "../std/map.hpp"
#include "../std/scoped_ptr.hpp"
#include "../std/string.hpp"
#include "../std/unordered_set.hpp"
#include "../std/vector.hpp"


class FeatureType;
//class Index;
//class MwmInfo;
class CategoriesHolder;

namespace storage { class CountryInfoGetter; }

namespace search
{

class LangKeywordsScorer;

namespace impl
{
  class FeatureLoader;
  class BestNameFinder;
  class PreResult2Maker;
}

class Query
{
public:
  static int const m_scaleDepthSearch = 7;

  // Vector of pairs (string_to_suggest, min_prefix_length_to_suggest).
  typedef vector<pair<strings::UniString, uint8_t> > StringsToSuggestVectorT;

  Query(Index const * pIndex,
        CategoriesHolder const * pCategories,
        StringsToSuggestVectorT const * pStringsToSuggest,
        storage::CountryInfoGetter const * pInfoGetter,
        size_t resultsNeeded = 10);
  ~Query();

  void SetViewport(m2::RectD viewport[], size_t count);

  static const int empty_pos_value = -1000;
  inline void SetPosition(m2::PointD const & pos) { m_position = pos; }
  inline void NullPosition() { m_position = m2::PointD(empty_pos_value, empty_pos_value); }

  inline void SetSearchInWorld(bool b) { m_worldSearch = b; }

  void SetPreferredLanguage(string const & lang);
  inline void SetInputLanguage(int8_t lang) { m_inputLang = lang; }

  void Search(string const & query, Results & res, unsigned int resultsNeeded = 10);
  void SearchAllInViewport(m2::RectD const & viewport, Results & res, unsigned int resultsNeeded = 30);
  void SearchAdditional(Results & res);

  void ClearCache();

  inline void DoCancel() { m_cancel = true; }
  inline bool IsCanceled() const { return m_cancel; }
  struct CancelException {};

private:
  friend class impl::FeatureLoader;
  friend class impl::BestNameFinder;
  friend class impl::PreResult2Maker;

  void InitSearch(string const & query);
  void InitKeywordsScorer();
  void ClearQueues();

  typedef vector<MwmInfo> MWMVectorT;
  typedef vector<vector<uint32_t> > OffsetsVectorT;

  void UpdateViewportOffsets(MWMVectorT const & mwmInfo, m2::RectD const & rect,
                             OffsetsVectorT & offsets);
  void ClearCache(size_t ind);

  typedef trie::ValueReader::ValueType TrieValueT;
  void AddResultFromTrie(TrieValueT const & val, size_t mwmID);

  void FlushResults(Results & res);

  struct Params
  {
    typedef vector<strings::UniString> TokensVectorT;
    typedef unordered_set<int8_t> LangsSetT;

    vector<TokensVectorT> m_tokens;
    TokensVectorT m_prefixTokens;
    LangsSetT m_langs;

    Params(Query & q);
  };

  void SearchFeatures();
  void SearchFeatures(Params const & params,
                      MWMVectorT const & mwmInfo,
                      size_t ind);
  void SearchInMWM(Index::MwmLock const & mwmLock,
                   Params const & params,
                   OffsetsVectorT const * offsets);

  void SuggestStrings(Results & res);
  void MatchForSuggestions(strings::UniString const & token, Results & res);

  void GetBestMatchName(FeatureType const & f, uint32_t & penalty, string & name);

  inline Result MakeResult(impl::PreResult2 const & r) const
  {
    return r.GenerateFinalResult(m_pInfoGetter, m_pCategories, m_currentLang);
  }

  Index const * m_pIndex;
  CategoriesHolder const * m_pCategories;
  StringsToSuggestVectorT const * m_pStringsToSuggest;
  storage::CountryInfoGetter const * m_pInfoGetter;
  int8_t m_currentLang, m_inputLang;

  volatile bool m_cancel;

  string m_rawQuery;
  strings::UniString m_uniQuery;
  buffer_vector<strings::UniString, 32> m_tokens;
  strings::UniString m_prefix;

  static size_t const RECTSCOUNT = 2;

  m2::RectD m_viewport[RECTSCOUNT];
  bool m_worldSearch;

  /// @return Rect for viewport-distance calculation.
  m2::RectD const & GetViewport() const;

  m2::PointD m_position;

  scoped_ptr<LangKeywordsScorer> m_pKeywordsScorer;

  OffsetsVectorT m_offsetsInViewport[RECTSCOUNT];

  template <class ParamT, class RefT> class CompareT
  {
    typedef bool (*FunctionT) (ParamT const &, ParamT const &);
    FunctionT m_fn;

  public:
    CompareT() : m_fn(0) {}
    explicit CompareT(FunctionT const & fn) : m_fn(fn) {}

    template <class T> bool operator() (T const & v1, T const & v2) const
    {
      RefT getR;
      return m_fn(getR(v1), getR(v2));
    }
  };

  struct NothingRef
  {
    template <class T> T const & operator() (T const & t) const { return t; }
  };
  struct RefPointer
  {
    template <class T> typename T::value_type const & operator() (T const & t) const { return *t; }
    template <class T> T const & operator() (T const * t) const { return *t; }
  };

  typedef CompareT<impl::PreResult1, NothingRef> QueueCompareT;
  typedef my::limited_priority_queue<impl::PreResult1, QueueCompareT> QueueT;

public:
  static const size_t m_qCount = 3;

private:
  QueueT m_results[m_qCount];
};

}  // namespace search
