#pragma once

#include "engine.hpp"
#include "intermediate_result.hpp"
#include "keyword_matcher.hpp"
#include "result.hpp"
#include "../geometry/rect2d.hpp"
#include "../base/string_utils.hpp"
#include "../std/function.hpp"
#include "../std/queue.hpp"
#include "../std/scoped_ptr.hpp"
#include "../std/string.hpp"
#include "../std/unordered_map.hpp"
#include "../std/vector.hpp"

namespace search
{

class CategoriesHolder;

namespace impl
{

class Query
{
public:
  typedef Engine::IndexType IndexType;

  Query(string const & query, m2::RectD const & viewport, IndexType const * pIndex,
        Engine * pEngine, CategoriesHolder * pCategories,
        TrieIterator * pTrieRoot, FeaturesVector * pFeatures);
  ~Query();

  // Search with parameters, passed in constructor.
  void Search(function<void (Result const &)> const & f);

  // Search and delete this.
  void SearchAndDestroy(function<void (Result const &)> const & f);

  // Add result for scoring.
  void AddResult(IntermediateResult const & result);

  // Send all accumulated results to f().
  void FlushResults(function<void (Result const &)> const & f);

  // Set a flag that query is not active any more and should terminate.
  void SetTerminateFlag() volatile { m_bTerminate = true; }
  bool GetTerminateFlag() const { return m_bTerminate; }

  string const & GetQueryText() const { return m_queryText; }
  strings::UniString const & GetQueryUniText() const { return m_queryUniText; }
  m2::RectD const & GetViewport() const { return m_viewport; }
  vector<strings::UniString> const & GetKeywords() const { return m_keywords; }
  strings::UniString const & GetPrefix() const { return m_prefix; }
  uint32_t GetKeywordsToSkipForType(uint32_t const type) const;

private:
  string m_queryText;
  strings::UniString m_queryUniText;
  m2::RectD m_viewport;
  CategoriesHolder * m_pCategories;
  TrieIterator * m_pTrieRoot;
  FeaturesVector * m_pFeatures;

  vector<strings::UniString> m_keywords;
  unordered_map<uint32_t, uint32_t> m_keywordsToSkipForType;
  strings::UniString m_prefix;

  scoped_ptr<IndexType const> m_pIndex;

  priority_queue<IntermediateResult> m_results;
  int m_resultsRemaining;

  Engine * m_pEngine;
  bool volatile m_bTerminate;
};

}  // namespace search::impl
}  // namespace search
