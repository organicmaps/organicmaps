#include "search/search_tests_support/test_search_request.hpp"

#include "search/params.hpp"
#include "search/search_tests_support/test_search_engine.hpp"

#include "base/logging.hpp"

namespace search
{
namespace tests_support
{
TestSearchRequest::TestSearchRequest(TestSearchEngine & engine, string const & query,
                                     string const & locale, Mode mode, m2::RectD const & viewport)
  : m_done(false)
{
  search::SearchParams params;
  params.m_query = query;
  params.m_inputLocale = locale;
  params.m_onStarted = bind(&TestSearchRequest::OnStarted, this);
  params.m_onResults = bind(&TestSearchRequest::OnResults, this, _1);
  params.SetMode(mode);
  engine.Search(params, viewport);
}

void TestSearchRequest::Wait()
{
  unique_lock<mutex> lock(m_mu);
  m_cv.wait(lock, [this]()
  {
    return m_done;
  });
}

steady_clock::duration TestSearchRequest::ResponseTime() const
{
  lock_guard<mutex> lock(m_mu);
  CHECK(m_done, ("This function may be called only when request is processed."));
  return m_endTime - m_startTime;
}

vector<search::Result> const & TestSearchRequest::Results() const
{
  lock_guard<mutex> lock(m_mu);
  CHECK(m_done, ("This function may be called only when request is processed."));
  return m_results;
}

void TestSearchRequest::OnStarted()
{
  lock_guard<mutex> lock(m_mu);
  m_startTime = m_timer.TimeElapsed();
}

void TestSearchRequest::OnResults(search::Results const & results)
{
  lock_guard<mutex> lock(m_mu);
  if (results.IsEndMarker())
  {
    m_done = true;
    m_endTime = m_timer.TimeElapsed();
    m_cv.notify_one();
  }
  else
  {
    m_results.assign(results.Begin(), results.End());
  }
}
}  // namespace tests_support
}  // namespace search
