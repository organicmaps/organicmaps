#pragma once

#include "search/cancel_exception.hpp"
#include "search/geocoder.hpp"
#include "search/intermediate_result.hpp"
#include "search/keyword_lang_matcher.hpp"
#include "search/mode.hpp"
#include "search/reverse_geocoder.hpp"
#include "search/suggest.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/feature_decl.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/string_utils.hpp"

#include "std/set.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

#define FIND_LOCALITY_TEST

#ifdef FIND_LOCALITY_TEST
#include "search/locality_finder.hpp"
#endif  // FIND_LOCALITY_TEST

class CategoriesHolder;
class Index;
namespace storage
{
class CountryInfoGetter;
}  // namespace storage

namespace search
{
class PreResult2Maker;

class Ranker
{
public:
  struct Params
  {
    bool m_viewportSearch = false;

    int8_t m_currentLocaleCode = CategoriesHolder::kEnglishCode;
    m2::RectD m_viewport;
    m2::PointD m_position;
    string m_pivotRegion;
    set<uint32_t> m_preferredTypes;
    bool m_suggestsEnabled = false;

    string m_query;
    buffer_vector<strings::UniString, 32> m_tokens;
    // Prefix of the last token in the query.
    // We need it here to make suggestions.
    strings::UniString m_prefix;

    int8_t m_categoryLocales[3];
    size_t m_numCategoryLocales = 0;
  };

  Ranker(PreRanker & preRanker, Index const & index, storage::CountryInfoGetter const & infoGetter,
         CategoriesHolder const & categories, vector<Suggest> const & suggests,
         my::Cancellable const & cancellable)
    : m_reverseGeocoder(index)
    , m_preRanker(preRanker)
    , m_cancellable(cancellable)
#ifdef FIND_LOCALITY_TEST
    , m_locality(&index)
#endif  // FIND_LOCALITY_TEST
    , m_index(index)
    , m_infoGetter(infoGetter)
    , m_categories(categories)
    , m_suggests(suggests)
  {
  }

  inline void Init(bool viewportSearch) { m_params.m_viewportSearch = viewportSearch; }

  bool IsResultExists(PreResult2 const & p, vector<IndexedValue> const & values);

  void MakePreResult2(Geocoder::Params const & params, vector<IndexedValue> & cont,
                      vector<FeatureID> & streets);

  Result MakeResult(PreResult2 const & r) const;
  void MakeResultHighlight(Result & res) const;

  void GetSuggestion(string const & name, string & suggest) const;
  void SuggestStrings(Results & res);
  void MatchForSuggestions(strings::UniString const & token, int8_t locale, string const & prolog,
                           Results & res);
  void GetBestMatchName(FeatureType const & f, string & name) const;
  void ProcessSuggestions(vector<IndexedValue> & vec, Results & res) const;

  void FlushResults(Geocoder::Params const & geocoderParams, Results & res, size_t resCount);
  void FlushViewportResults(Geocoder::Params const & geocoderParams, Results & res);

  void SetParams(Params const & params) { m_params = params; }

  void ClearCaches();

  void SetLocalityFinderLanguage(int8_t code);

  inline void BailIfCancelled() { ::search::BailIfCancelled(m_cancellable); }

  KeywordLangMatcher m_keywordsScorer;

  friend class PreResult2Maker;

private:
  Params m_params;
  ReverseGeocoder const m_reverseGeocoder;
  PreRanker & m_preRanker;
  my::Cancellable const & m_cancellable;

#ifdef FIND_LOCALITY_TEST
  mutable LocalityFinder m_locality;
#endif  // FIND_LOCALITY_TEST

  Index const & m_index;
  storage::CountryInfoGetter const & m_infoGetter;
  CategoriesHolder const & m_categories;
  vector<Suggest> const & m_suggests;
};
}  // namespace search
