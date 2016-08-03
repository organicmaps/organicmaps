#pragma once

#include "geometry/rect2d.hpp"

#include "search/params.hpp"
#include "search/result.hpp"

#include "std/condition_variable.hpp"
#include "std/mutex.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

#include "base/timer.hpp"

namespace search
{
namespace tests_support
{
class TestSearchEngine;

// This class wraps a search query to a search engine. Note that the
// last token will be handled as a complete token iff it will be
// followed by a space in a query.
class TestSearchRequest
{
public:
  TestSearchRequest(TestSearchEngine & engine, string const & query, string const & locale,
                    Mode mode, m2::RectD const & viewport);
  TestSearchRequest(TestSearchEngine & engine, SearchParams params, m2::RectD const & viewport);

  void Wait();

  // Call these functions only after call to Wait().
  steady_clock::duration ResponseTime() const;
  vector<search::Result> const & Results() const;

protected:
  TestSearchRequest(TestSearchEngine & engine, string const & query, string const & locale,
                    Mode mode, m2::RectD const & viewport, TOnStarted onStarted,
                    TOnResults onResults);

  void SetUpCallbacks(SearchParams & params);

  void OnStarted();
  void OnResults(search::Results const & results);

  condition_variable m_cv;
  mutable mutex m_mu;

  vector<search::Result> m_results;
  bool m_done = false;

  my::Timer m_timer;
  steady_clock::duration m_startTime;
  steady_clock::duration m_endTime;
};
}  // namespace tests_support
}  // namespace search
