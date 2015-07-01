#include "search/integration_tests/test_search_request.hpp"

#include "search/integration_tests/test_search_engine.hpp"
#include "search/params.hpp"

#include "base/logging.hpp"

TestSearchRequest::TestSearchRequest(TestSearchEngine & engine, string const & query,
                                     string const & locale, m2::RectD const & viewport)
    : m_done(false)
{
  search::SearchParams params;
  params.m_query = query;
  params.m_inputLocale = locale;
  params.m_callback = [this](search::Results const & results)
  {
    Done(results);
  };
  params.SetSearchMode(search::SearchParams::IN_VIEWPORT_ONLY);
  CHECK(engine.Search(params, viewport), ("Can't run search."));
}

void TestSearchRequest::Wait()
{
  unique_lock<mutex> lock(m_mu);
  m_cv.wait(lock, [this]()
            {
    return m_done;
  });
}

vector<search::Result> const & TestSearchRequest::Results() const
{
  lock_guard<mutex> lock(m_mu);
  CHECK(m_done, ("Results can be get only when request will be completed."));
  return m_results;
}

void TestSearchRequest::Done(search::Results const & results)
{
  lock_guard<mutex> lock(m_mu);
  m_results.insert(m_results.end(), results.Begin(), results.End());
  if (results.IsEndMarker())
  {
    m_done = true;
    m_cv.notify_one();
  }
}
