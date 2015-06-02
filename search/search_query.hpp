#pragma once
#include "intermediate_result.hpp"
#include "keyword_lang_matcher.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/search_trie.hpp"
#include "indexer/index.hpp"   // for Index::MwmLock

#include "geometry/rect2d.hpp"

#include "base/buffer_vector.hpp"
#include "base/limited_priority_queue.hpp"
#include "base/string_utils.hpp"

#include "std/map.hpp"
#include "std/string.hpp"
#include "std/unordered_set.hpp"
#include "std/vector.hpp"


#define HOUSE_SEARCH_TEST
#define FIND_LOCALITY_TEST

#ifdef HOUSE_SEARCH_TEST
#include "search/house_detector.hpp"
#endif

#ifdef FIND_LOCALITY_TEST
#include "search/locality_finder.hpp"
#endif


class FeatureType;
class CategoriesHolder;

namespace storage { class CountryInfoGetter; }

namespace search
{

namespace impl
{
  class FeatureLoader;
  class BestNameFinder;
  class PreResult2Maker;
  struct Locality;
  struct Region;
  class DoFindLocality;
  class HouseCompFactory;
}

class Query
{
public:
  struct SuggestT
  {
    strings::UniString m_name;
    uint8_t m_prefixLength;
    int8_t m_locale;

    SuggestT(strings::UniString const & name, uint8_t len, int8_t locale)
      : m_name(name), m_prefixLength(len), m_locale(locale)
    {
    }
  };

  // Vector of suggests.
  typedef vector<SuggestT> StringsToSuggestVectorT;

  Query(Index const * pIndex,
        CategoriesHolder const * pCategories,
        StringsToSuggestVectorT const * pStringsToSuggest,
        storage::CountryInfoGetter const * pInfoGetter);
  ~Query();

  inline void SupportOldFormat(bool b) { m_supportOldFormat = b; }

  void Init(bool viewportSearch);

  /// @param[in]  forceUpdate Pass true (default) to recache feature's ids even
  /// if viewport is a part of the old cached rect.
  void SetViewport(m2::RectD const & viewport, bool forceUpdate);
  void SetRankPivot(m2::PointD const & pivot);
  inline string const & GetPivotRegion() const { return m_region; }

  inline void SetSearchInWorld(bool b) { m_worldSearch = b; }

  /// Suggestions language code, not the same as we use in mwm data
  int8_t m_inputLocaleCode, m_currentLocaleCode;

  void SetPreferredLocale(string const & locale);
  void SetInputLocale(string const & locale);

  void SetQuery(string const & query);
  inline bool IsEmptyQuery() const { return (m_prefix.empty() && m_tokens.empty()); }

  /// @name Different search functions.
  //@{
  void SearchCoordinates(string const & query, Results & res) const;
  void Search(Results & res, size_t resCount);
  void SearchAdditional(Results & res, size_t resCount);

  void SearchViewportPoints(Results & res);
  //@}

  void ClearCaches();

  inline void DoCancel() { m_cancel = true; }
  inline bool IsCanceled() const { return m_cancel; }
  struct CancelException {};

  /// @name This stuff is public for implementation classes in search_query.cpp
  /// Do not use it in client code.
  //@{
  typedef trie::ValueReader::ValueType TrieValueT;

  struct Params
  {
    typedef strings::UniString StringT;
    typedef vector<StringT> TokensVectorT;
    typedef unordered_set<int8_t> LangsSetT;

    vector<TokensVectorT> m_tokens;
    TokensVectorT m_prefixTokens;
    LangsSetT m_langs;

    /// Initialize search params (tokens, languages).
    /// @param[in]  isLocalities  Use true when search for locality in World.
    Params(Query const & q, bool isLocalities = false);

    /// @param[in] eraseInds Sorted vector of token's indexes.
    void EraseTokens(vector<size_t> & eraseInds);

    void ProcessAddressTokens();

    bool IsEmpty() const { return (m_tokens.empty() && m_prefixTokens.empty()); }
    bool CanSuggest() const { return (m_tokens.empty() && !m_prefixTokens.empty()); }
    bool IsLangExist(int8_t l) const { return (m_langs.count(l) > 0); }

  private:
    template <class ToDo> void ForEachToken(ToDo toDo);

    void FillLanguages(Query const & q);
  };
  //@}

private:
  friend class impl::FeatureLoader;
  friend class impl::BestNameFinder;
  friend class impl::PreResult2Maker;
  friend class impl::DoFindLocality;
  friend class impl::HouseCompFactory;

  void ClearQueues();

  int GetCategoryLocales(int8_t (&arr) [3]) const;
  template <class ToDo> void ForEachCategoryTypes(ToDo toDo) const;

  typedef vector<shared_ptr<MwmInfo>> MWMVectorT;
  typedef map<MwmSet::MwmId, vector<uint32_t>> OffsetsVectorT;
  typedef feature::DataHeader FHeaderT;

  void SetViewportByIndex(MWMVectorT const & mwmsInfo, m2::RectD const & viewport,
                          size_t idx, bool forceUpdate);
  void UpdateViewportOffsets(MWMVectorT const & mwmsInfo, m2::RectD const & rect,
                             OffsetsVectorT & offsets);
  void ClearCache(size_t ind);

  enum ViewportID
  {
    DEFAULT_V = -1,
    CURRENT_V = 0,
    LOCALITY_V = 1,
    COUNT_V = 2     // Should always be the last
  };

  void AddResultFromTrie(TrieValueT const & val, MwmSet::MwmId const & mwmID,
                         ViewportID vID = DEFAULT_V);

  template <class T> void MakePreResult2(vector<T> & cont, vector<FeatureID> & streets);
  void FlushHouses(Results & res, bool allMWMs, vector<FeatureID> const & streets);
  void FlushResults(Results & res, bool allMWMs, size_t resCount);

  ftypes::Type GetLocalityIndex(feature::TypesHolder const & types) const;
  void RemoveStringPrefix(string const & str, string & res) const;
  void GetSuggestion(string const & name, string & suggest) const;
  template <class T> void ProcessSuggestions(vector<T> & vec, Results & res) const;

  void SearchAddress(Results & res);

  /// Search for best localities by input tokens.
  /// @param[in]  pMwm  MWM file for World
  /// @param[out] res1  Best city-locality
  /// @param[out] res2  Best region-locality
  void SearchLocality(MwmValue * pMwm, impl::Locality & res1, impl::Region & res2);

  void SearchFeatures();

  /// @param[in] ind Index of viewport rect to search (@see m_viewport).
  /// If ind == -1, don't do any matching with features in viewport (@see m_offsetsInViewport).
  //@{
  /// Do search in all maps from mwmInfo.
  void SearchFeatures(Params const & params, MWMVectorT const & mwmsInfo, ViewportID vID);
  /// Do search in particular map (mwmLock).
  void SearchInMWM(Index::MwmLock const & mwmLock, Params const & params, ViewportID vID = DEFAULT_V);
  //@}

  void SuggestStrings(Results & res);
  void MatchForSuggestionsImpl(strings::UniString const & token, int8_t locale, string const & prolog, Results & res);

  void GetBestMatchName(FeatureType const & f, string & name) const;

  Result MakeResult(impl::PreResult2 const & r) const;
  void MakeResultHighlight(Result & res) const;

  Index const * m_pIndex;
  CategoriesHolder const * m_pCategories;
  StringsToSuggestVectorT const * m_pStringsToSuggest;
  storage::CountryInfoGetter const * m_pInfoGetter;

  volatile bool m_cancel;

  string m_region;
  string const * m_query;
  buffer_vector<strings::UniString, 32> m_tokens;
  strings::UniString m_prefix;
  set<uint32_t> m_prefferedTypes;

#ifdef HOUSE_SEARCH_TEST
  strings::UniString m_house;
  vector<FeatureID> m_streetID;
  HouseDetector m_houseDetector;
#endif

#ifdef FIND_LOCALITY_TEST
  LocalityFinder m_locality;
#endif

  m2::RectD m_viewport[COUNT_V];
  m2::PointD m_pivot;
  bool m_worldSearch;

  /// @name Get ranking params.
  //@{
  /// @return Rect for viewport-distance calculation.
  m2::RectD const & GetViewport(ViewportID vID = DEFAULT_V) const;
  /// @return Control point for distance-to calculation.
  m2::PointD GetPosition(ViewportID vID = DEFAULT_V) const;
  //@}

  void SetLanguage(int id, int8_t lang);
  int8_t GetLanguage(int id) const;

  KeywordLangMatcher m_keywordsScorer;

  OffsetsVectorT m_offsetsInViewport[COUNT_V];
  bool m_supportOldFormat;

  template <class ParamT> class CompareT
  {
    typedef bool (*FunctionT) (ParamT const &, ParamT const &);
    FunctionT m_fn;

  public:
    CompareT() : m_fn(0) {}
    explicit CompareT(FunctionT const & fn) : m_fn(fn) {}

    template <class T> bool operator() (T const & v1, T const & v2) const
    {
      return m_fn(v1, v2);
    }
  };

  typedef CompareT<impl::PreResult1> QueueCompareT;
  typedef my::limited_priority_queue<impl::PreResult1, QueueCompareT> QueueT;

public:
  enum { QUEUES_COUNT = 2 };
private:
  // 0 - LessDistance
  // 1 - LessRank
  QueueT m_results[QUEUES_COUNT];
  size_t m_queuesCount;
};

}  // namespace search
