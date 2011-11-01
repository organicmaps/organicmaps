#pragma once
#include "intermediate_result.hpp"
#include "../geometry/rect2d.hpp"
#include "../base/buffer_vector.hpp"
#include "../base/limited_priority_queue.hpp"
#include "../base/string_utils.hpp"
#include "../std/function.hpp"
#include "../std/map.hpp"
#include "../std/scoped_ptr.hpp"
#include "../std/string.hpp"
#include "../std/unordered_set.hpp"
#include "../std/vector.hpp"


class FeatureType;
class Index;

namespace storage { class CountryInfoGetter; }

namespace search
{

struct CategoryInfo;
class KeywordMatcher;
namespace impl { class IntermediateResult; struct FeatureLoader; class BestNameFinder; }

class Query
{
public:
  // Map category_token -> category_type.
  typedef multimap<strings::UniString, uint32_t> CategoriesMapT;
  // Vector of pairs (string_to_suggest, min_prefix_length_to_suggest).
  typedef vector<pair<strings::UniString, uint8_t> > StringsToSuggestVectorT;

  Query(Index const * pIndex,
        CategoriesMapT const * pCategories,
        StringsToSuggestVectorT const * pStringsToSuggest,
        storage::CountryInfoGetter const * pInfoGetter);
  ~Query();

  void SetViewport(m2::RectD const & viewport);
  void Search(string const & query,
              function<void (Result const &)> const & f,
              unsigned int resultsNeeded = 10);

  void ClearCache();

private:

  friend struct impl::FeatureLoader;
  friend class impl::BestNameFinder;

  void AddResult(impl::IntermediateResult const & result);
  void AddFeatureResult(FeatureType const & f, string const & fName);
  void FlushResults(function<void (Result const &)> const & f);
  void UpdateViewportOffsets();
  void SearchFeatures();
  void SuggestStrings();

  void GetBestMatchName(FeatureType const & f, uint32_t & penalty, string & name);
  string GetRegionName(FeatureType const & f, string const & fName);

  Index const * m_pIndex;
  CategoriesMapT const * m_pCategories;
  StringsToSuggestVectorT const * m_pStringsToSuggest;
  storage::CountryInfoGetter const * m_pInfoGetter;

  string m_rawQuery;
  strings::UniString m_uniQuery;
  buffer_vector<strings::UniString, 32> m_tokens;
  strings::UniString m_prefix;
  m2::RectD m_viewport;
  m2::RectD m_viewportExtended;

  scoped_ptr<KeywordMatcher> m_pKeywordMatcher;

  bool m_bOffsetsCacheIsValid;
  vector<unordered_set<uint32_t> > m_offsetsInViewport;

  my::limited_priority_queue<impl::IntermediateResult> m_results;
};

}  // namespace search
