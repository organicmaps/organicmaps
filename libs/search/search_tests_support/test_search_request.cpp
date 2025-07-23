#include "search/search_tests_support/test_search_request.hpp"

#include "search/search_tests_support/test_search_engine.hpp"

#include "base/assert.hpp"

#include <functional>

namespace search
{
namespace tests_support
{
using namespace std::chrono;
using namespace std;

TestSearchRequest::TestSearchRequest(TestSearchEngine & engine, string const & query, string const & locale, Mode mode,
                                     m2::RectD const & viewport)
  : m_engine(engine)
{
  m_params.m_query = query;
  m_params.m_inputLocale = locale;
  m_params.m_viewport = viewport;
  m_params.m_mode = mode;
  m_params.m_useDebugInfo = true;

  SetUpCallbacks();
  SetUpResultParams();
}

TestSearchRequest::TestSearchRequest(TestSearchEngine & engine, SearchParams const & params)
  : m_engine(engine)
  , m_params(params)
{
  m_params.m_useDebugInfo = true;

  SetUpCallbacks();
}

TestSearchRequest::TestSearchRequest(TestSearchEngine & engine, string const & query, string const & locale, Mode mode,
                                     m2::RectD const & viewport, SearchParams::OnStarted const & onStarted,
                                     SearchParams::OnResults const & onResults)
  : m_engine(engine)
{
  m_params.m_query = query;
  m_params.m_inputLocale = locale;
  m_params.m_viewport = viewport;
  m_params.m_mode = mode;
  m_params.m_onStarted = onStarted;
  m_params.m_onResults = onResults;
  m_params.m_useDebugInfo = true;

  SetUpResultParams();
}

void TestSearchRequest::Run()
{
  Start();
  Wait();
}

TestSearchRequest::TimeDurationT TestSearchRequest::ResponseTime() const
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
  m_params.m_onResults = bind(&TestSearchRequest::OnResults, this, placeholders::_1);
}

void TestSearchRequest::SetUpResultParams()
{
  switch (m_params.m_mode)
  {
  case Mode::Everywhere:
    m_params.m_needAddress = true;
    m_params.m_suggestsEnabled = false;
    m_params.m_needHighlighting = true;
    break;
  case Mode::Viewport:    // fallthrough
  case Mode::Downloader:  // fallthrough
  case Mode::Bookmarks:
    m_params.m_needAddress = false;
    m_params.m_suggestsEnabled = false;
    m_params.m_needHighlighting = false;
    break;
  case Mode::Count: CHECK(false, ()); break;
  }
}

void TestSearchRequest::OnStarted()
{
  lock_guard<mutex> lock(m_mu);
  m_startTime = m_timer.TimeElapsed();
}

void TestSearchRequest::OnResults(search::Results const & results)
{
  lock_guard<mutex> lock(m_mu);
  m_results.assign(results.begin(), results.end());
  if (results.IsEndMarker())
  {
    m_done = true;
    m_endTime = m_timer.TimeElapsed();
    m_cv.notify_one();
  }
}

void TestSearchRequest::SetCustomOnResults(SearchParams::OnResults const & onResults)
{
  m_params.m_onResults = onResults;
}
}  // namespace tests_support
}  // namespace search
