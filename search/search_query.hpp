#pragma once
#include "intermediate_result.hpp"
#include "../geometry/rect2d.hpp"
#include "../base/buffer_vector.hpp"
#include "../base/limited_priority_queue.hpp"
#include "../std/function.hpp"
#include "../std/string.hpp"
#include "../std/unordered_set.hpp"
#include "../std/vector.hpp"

class Index;

namespace search
{

class CategoriesHolder;
namespace impl { class IntermediateResult; class FeatureLoader; }

class Query
{
public:
  Query(Index const * pIndex, CategoriesHolder const * pCategories);
  ~Query();

  void SetViewport(m2::RectD const & viewport);
  void Search(string const & query,
              function<void (Result const &)> const & f,
              unsigned int resultsNeeded = 10);

private:

  friend class impl::FeatureLoader;

  void AddResult(impl::IntermediateResult const & result);
  void FlushResults(function<void (Result const &)> const & f);
  void UpdateViewportOffsets();
  void SearchFeatures();

  Index const * m_pIndex;
  CategoriesHolder const * m_pCategories;

  string m_rawQuery;
  strings::UniString m_uniQuery;
  buffer_vector<strings::UniString, 32> m_tokens;
  strings::UniString m_prefix;
  m2::RectD m_viewport;
  m2::RectD m_viewportExtended;

  vector<unordered_set<uint32_t> > m_offsetsInViewport;

  my::limited_priority_queue<impl::IntermediateResult> m_results;
};

}  // namespace search
