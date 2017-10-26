#include "search/search_tests_support/test_search_request.hpp"

#include "search/search_tests_support/test_search_engine.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"

#include "base/logging.hpp"

namespace search
{
namespace tests_support
{
TestSearchRequest::TestSearchRequest(TestSearchEngine & engine, string const & query,
                                     string const & locale, Mode mode, m2::RectD const & viewport)
  : m_engine(engine)
{
  m_params.m_query = query;
  m_params.m_inputLocale = locale;
  m_params.m_viewport = viewport;
  m_params.m_mode = mode;
  SetUpCallbacks();
}

TestSearchRequest::TestSearchRequest(TestSearchEngine & engine, SearchParams const & params)
  : m_engine(engine), m_params(params)
{
  SetUpCallbacks();
}

TestSearchRequest::TestSearchRequest(TestSearchEngine & engine, string const & query,
                                     string const & locale, Mode mode, m2::RectD const & viewport,
                                     SearchParams::OnStarted const & onStarted,
                                     SearchParams::OnResults const & onResults)
  : m_engine(engine)
{
  m_params.m_query = query;
  m_params.m_inputLocale = locale;
  m_params.m_viewport = viewport;
  m_params.m_mode = mode;
  m_params.m_onStarted = onStarted;
  m_params.m_onResults = onResults;
}

void TestSearchRequest::Run()
{
  Start();
  Wait();
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

void TestSearchRequest::Start()
{
  m_engine.Search(m_params);
}

void TestSearchRequest::Wait()
{
  unique_lock<mutex> lock(m_mu);
  m_cv.wait(lock, [this]() { return m_done; });
}

void TestSearchRequest::SetUpCallbacks()
{
  m_params.m_onStarted = bind(&TestSearchRequest::OnStarted, this);
  m_params.m_onResults = bind(&TestSearchRequest::OnResults, this, _1);
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
    m_results.assign(results.begin(), results.end());
  }
}

void TestSearchRequest::SetCustomOnResults(SearchParams::OnResults const & onResults)
{
  m_params.m_onResults = onResults;
}
}  // namespace tests_support
}  // namespace search
