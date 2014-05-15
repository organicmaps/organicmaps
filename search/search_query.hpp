#pragma once
#include "intermediate_result.hpp"
#include "keyword_lang_matcher.hpp"
#include "ftypes_matcher.hpp"

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


#define HOUSE_SEARCH_TEST

#ifdef HOUSE_SEARCH_TEST
#include "house_detector.hpp"
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
  static int const SCALE_SEARCH_DEPTH = 7;
  static int const ADDRESS_SCALE = 10;

  struct SuggestT
  {
    strings::UniString m_name;
    uint8_t m_prefixLength;
    int8_t m_lang;

    SuggestT(strings::UniString const & name, uint8_t len, int8_t lang)
      : m_name(name), m_prefixLength(len), m_lang(lang)
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

  void Init();

  void SetViewport(m2::RectD viewport[], size_t count);

  static const int empty_pos_value = -1000;
  inline void SetPosition(m2::PointD const & pos) { m_position = pos; }
  inline void NullPosition() { m_position = m2::PointD(empty_pos_value, empty_pos_value); }

  inline void SetSearchInWorld(bool b) { m_worldSearch = b; }

  void SetPreferredLanguage(string const & lang);
  void SetInputLanguage(int8_t lang);
  int8_t GetPrefferedLanguage() const;

  void SetQuery(string const & query);
  inline bool IsEmptyQuery() const { return (m_prefix.empty() && m_tokens.empty()); }

  /// @name Different search functions.
  //@{
  void SearchCoordinates(string const & query, Results & res) const;
  void Search(Results & res, size_t resCount);
  void SearchAllInViewport(m2::RectD const & viewport, Results & res, unsigned int resultsNeeded = 30);
  void SearchAdditional(Results & res, bool nearMe, bool inViewport, size_t resCount);
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

  typedef vector<MwmInfo> MWMVectorT;
  typedef vector<vector<uint32_t> > OffsetsVectorT;
  typedef feature::DataHeader FHeaderT;

  void SetViewportByIndex(MWMVectorT const & mwmInfo, m2::RectD const & viewport, size_t idx);
  void UpdateViewportOffsets(MWMVectorT const & mwmInfo, m2::RectD const & rect,
                             OffsetsVectorT & offsets);
  void ClearCache(size_t ind);

  /// @param[in]  viewportID  @see m_viewport
  void AddResultFromTrie(TrieValueT const & val, size_t mwmID, int8_t viewportID = -1);

  void FlushResults(Results & res, bool allMWMs, size_t resCount);

  ftypes::Type GetLocalityIndex(feature::TypesHolder const & types) const;
  void GetSuggestion(string const & name, string & suggest) const;
  template <class T> void ProcessSuggestions(vector<T> & vec, Results & res) const;

  void SearchAddress();

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
  void SearchFeatures(Params const & params, MWMVectorT const & mwmInfo, int ind);
  /// Do search in particular map (mwmLock).
  void SearchInMWM(Index::MwmLock const & mwmLock, Params const & params, int ind = -1);
  //@}

  void SuggestStrings(Results & res);
  bool MatchForSuggestionsImpl(strings::UniString const & token, int8_t lang, Results & res);
  void MatchForSuggestions(strings::UniString const & token, Results & res);

  void GetBestMatchName(FeatureType const & f, string & name) const;

  Result MakeResult(impl::PreResult2 const & r) const;

  Index const * m_pIndex;
  CategoriesHolder const * m_pCategories;
  StringsToSuggestVectorT const * m_pStringsToSuggest;
  storage::CountryInfoGetter const * m_pInfoGetter;

  volatile bool m_cancel;

  string const * m_query;
  buffer_vector<strings::UniString, 32> m_tokens;
  strings::UniString m_prefix;
  set<uint32_t> m_prefferedTypes;

#ifdef HOUSE_SEARCH_TEST
  strings::UniString m_house;
  vector<FeatureID> m_streetID;
  search::HouseDetector m_houseDetector;
#endif

  static int const MAX_SUGGESTS_COUNT = 5;

  /// 0 - current viewport rect
  /// 1 - near me rect
  /// 2 - around city rect
  static size_t const RECTSCOUNT = 3;
  static int const ADDRESS_RECT_ID = RECTSCOUNT-1;

  m2::RectD m_viewport[RECTSCOUNT];
  bool m_worldSearch;

  /// @name Get ranking params.
  /// @param[in]  viewportID  Index of search viewport (@see comments above); -1 means default viewport.
  //@{
  /// @return Rect for viewport-distance calculation.
  m2::RectD const & GetViewport(int8_t viewportID = -1) const;
  m2::PointD GetPosition(int8_t viewportID = -1) const;
  //@}

  m2::PointD m_position;
  bool IsValidPosition() const;

  void SetLanguage(int id, int8_t lang);
  int8_t GetLanguage(int id) const;

  KeywordLangMatcher m_keywordsScorer;

  OffsetsVectorT m_offsetsInViewport[RECTSCOUNT];
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
  static const size_t m_qCount = 3;

private:
  QueueT m_results[m_qCount];
};

}  // namespace search
